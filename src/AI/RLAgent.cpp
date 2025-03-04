#include "RLAgent.hpp"
#include "Core/Logger.hpp"
#include <algorithm>
#include <cmath>

static constexpr float DEFAULT_EPISODE_DURATION = 60.0f;
static constexpr int TARGET_UPDATE_FREQUENCY = 1000;

static ActionType animationKeyToActionType(const std::string &animKey) {

  if (animKey.find("Attack") != std::string::npos)
    return ActionType::Attack;
  if (animKey.find("Block") != std::string::npos)
    return ActionType::Block;
  if (animKey.find("Jump") != std::string::npos)
    return ActionType::Jump;
  if (animKey.find("Dash") != std::string::npos)
    return ActionType::MoveRight;

  return ActionType::Noop;
}

RLAgent::RLAgent(Character *character, Config &config)
    : m_character(character), m_totalReward(0), m_episodeTime(0),
      m_episodeDuration(DEFAULT_EPISODE_DURATION), m_timeSinceLastAction(0),
      m_lastHealth(100), m_currentActionDuration(0), m_actionHoldDuration(0.2f),
      m_consecutiveWhiffs(0), m_lastOpponentHealth(0), m_epsilon(0.3f),
      m_learningRate(0.001f), m_discountFactor(0.95f), m_episodeCount(0),
      updateCounter(0), m_wins(0), m_totalRounds(0), m_winRate(0.0f),
      m_gen(m_rd()), m_dist(0.0f, 1.0f), m_moveHoldCounter(0),
      m_currentStance(Stance::Neutral), m_comboCount(0), m_config(config) {

  state_dim = 14;
  num_actions = 9;

  onlineDQN = std::make_unique<NeuralNetwork>(state_dim);
  onlineDQN->addLayer(64, ActivationType::Sigmoid);
  onlineDQN->addLayer(num_actions, ActivationType::None);

  targetDQN = std::make_unique<NeuralNetwork>(state_dim);
  targetDQN->addLayer(64, ActivationType::Sigmoid);
  targetDQN->addLayer(num_actions, ActivationType::None);

  m_epsilon = 1.0f;
  m_epsilon_min = 0.01f;
  m_epsilon_decay = 0.995f;
  m_learningRate = 0.0005f;
  m_discountFactor = 0.99f;

  m_battleStyle.timePenalty = 0.004f;
  m_battleStyle.hpRatioWeight = 1.0f;
  m_battleStyle.distancePenalty = 0.0002f;

  m_lastOpponentPosition = m_character->mover.position;
  m_opponentVelocity = Vector2f(0, 0);

  reset();
}

std::vector<float> RLAgent::stateToVector(const State &state) {
  std::vector<float> v;

  std::vector<float> ranges = StateNormalization::getNormalizationRanges();

  v.push_back(state.distanceToOpponent / ranges[0]);
  v.push_back(state.relativePositionX / ranges[1]);
  v.push_back(state.relativePositionY / ranges[2]);
  v.push_back(state.myHealth);
  v.push_back(state.opponentHealth);
  v.push_back(state.timeSinceLastAction / ranges[5]);

  for (int i = 0; i < 4; ++i) {
    v.push_back(state.radar[i] / ranges[6 + i]);
  }

  v.push_back(state.opponentVelocityX / ranges[10]);
  v.push_back(state.opponentVelocityY / ranges[11]);

  v.push_back(state.isCornered ? 1.0f : 0.0f);
  v.push_back(static_cast<float>(state.currentStance) / 2.0f);
  v.push_back(state.myStamina);
  v.push_back(state.myMaxStamina);

  return v;
}

State RLAgent::getCurrentState(const Character &opponent) {
  State state;
  Vector2f toOpponent = opponent.mover.position - m_character->mover.position;
  state.distanceToOpponent = toOpponent.length();

  int facing = m_character->animator->getFlip() ? -1 : 1;
  state.relativePositionX = toOpponent.x * facing;
  state.relativePositionY = toOpponent.y;
  state.myHealth =
      static_cast<float>(m_character->health) / m_character->maxHealth;
  state.opponentHealth =
      static_cast<float>(opponent.health) / opponent.maxHealth;
  state.timeSinceLastAction = m_timeSinceLastAction;

  state.radar[0] =
      (toOpponent.x >= 0 && toOpponent.y >= 0) ? toOpponent.length() : 0;
  state.radar[1] =
      (toOpponent.x < 0 && toOpponent.y >= 0) ? toOpponent.length() : 0;
  state.radar[2] =
      (toOpponent.x < 0 && toOpponent.y < 0) ? toOpponent.length() : 0;
  state.radar[3] =
      (toOpponent.x >= 0 && toOpponent.y < 0) ? toOpponent.length() : 0;

  state.opponentVelocityX = m_opponentVelocity.x;
  state.opponentVelocityY = m_opponentVelocity.y;

  float posX = m_character->mover.position.x;
  state.isCornered =
      (posX < m_config.ai.deadzoneBoundary ||
       posX > m_config.windowWidth - m_config.ai.deadzoneBoundary);

  std::array<ActionType, 10> selfHist = {ActionType::Noop, ActionType::Noop,
                                         ActionType::Noop};
  std::array<ActionType, 10> oppHist = {ActionType::Noop, ActionType::Noop,
                                        ActionType::Noop};
  int i = 0;
  for (auto it = m_actionHistory.rbegin();
       it != m_actionHistory.rend() && i < 10; ++it, ++i)
    selfHist[i] = *it;
  i = 0;
  for (auto it = m_opponentActionHistory.rbegin();
       it != m_opponentActionHistory.rend() && i < 10; ++it, ++i)
    oppHist[i] = *it;
  state.lastActions = selfHist;
  state.opponentLastActions = oppHist;

  Vector2f predictedPos = opponent.mover.position + m_opponentVelocity * 0.5f;
  state.predictedDistance =
      (predictedPos - m_character->mover.position).length();

  state.currentStance = m_currentStance;

  state.myStamina = m_character->stamina / m_character->maxStamina;
  state.myMaxStamina = 1.0f;

  return state;
}

Action RLAgent::selectAction(const State &state) {
  auto state_vec = stateToVector(state);
  auto q_values = onlineDQN->forward(state_vec);
  Action selectedAction;

  float situationalEpsilon = m_epsilon;
  if (state.myHealth < 0.3f || state.isCornered) {
    situationalEpsilon *= 1.5f;
  }

  if (m_dist(m_gen) < situationalEpsilon) {

    std::vector<ActionType> validActions;
    for (int i = 0; i < num_actions; i++) {
      validActions.push_back(static_cast<ActionType>(i));
    }

    if (state.isCornered) {

      float posX = m_character->mover.position.x;
      if (posX < 150.f) {
        validActions.erase(std::remove(validActions.begin(), validActions.end(),
                                       ActionType::MoveLeft),
                           validActions.end());
      } else if (posX > 850.f) {
        validActions.erase(std::remove(validActions.begin(), validActions.end(),
                                       ActionType::MoveRight),
                           validActions.end());
      }
    }

    if (m_character->stamina < 20.f) {

      validActions.erase(std::remove(validActions.begin(), validActions.end(),
                                     ActionType::Attack),
                         validActions.end());
      validActions.erase(std::remove(validActions.begin(), validActions.end(),
                                     ActionType::JumpAttack),
                         validActions.end());
    }

    int randomIndex = static_cast<int>(m_dist(m_gen) * validActions.size());
    selectedAction = Action::fromType(validActions[randomIndex]);
  } else {
    int bestAction = std::distance(
        q_values.begin(), std::max_element(q_values.begin(), q_values.end()));
    selectedAction = Action::fromType(static_cast<ActionType>(bestAction));
  }

  Action predictedOppAction = predictOpponentAction(state);

  if (state.myHealth < state.opponentHealth * 0.3f ||
      m_currentStance == Stance::Defensive) {
    if (predictedOppAction.type == ActionType::Attack && m_dist(m_gen) < 0.8f) {
      selectedAction = Action::fromType(ActionType::Block);
    }
  }

  float posX = m_character->mover.position.x;
  if (posX < m_config.ai.deadzoneBoundary) {
    selectedAction = Action::fromType(ActionType::MoveRight);
  } else if (posX > m_config.windowWidth - m_config.ai.deadzoneBoundary) {
    selectedAction = Action::fromType(ActionType::MoveLeft);
  }

  if (static_cast<size_t>(selectedAction.type) < q_values.size()) {
    m_qValueHistory.push_back(q_values[static_cast<int>(selectedAction.type)]);
    if (m_qValueHistory.size() > 100) {
      m_qValueHistory.erase(m_qValueHistory.begin());
    }
  }

  return selectedAction;
}

float RLAgent::calculateReward(const State &state, const Action &action) {
  float reward = 0.0f;

  float healthDiff = state.myHealth - state.opponentHealth;
  reward += healthDiff * m_config.ai.healthDiffReward;

  float posX = m_character->mover.position.x;

  if (posX < m_config.ai.deadzoneBoundary ||
      posX > m_config.windowWidth - m_config.ai.deadzoneBoundary) {

    float distanceFromCenter = std::abs(posX - (m_config.windowWidth / 2.0f));
    float deadzoneDepth = std::min(m_config.ai.deadzoneBoundary,
                                   std::min(posX, m_config.windowWidth - posX));

    reward += m_config.ai.deadzoneBasePenalty +
              (1.0f - (deadzoneDepth / m_config.ai.deadzoneBoundary)) *
                  m_config.ai.deadzoneDepthPenalty;

    if ((posX < m_config.ai.deadzoneBoundary && action.moveLeft) ||
        (posX > m_config.windowWidth - m_config.ai.deadzoneBoundary &&
         action.moveRight)) {
      reward += m_config.ai.moveIntoDeadzonePenalty;
    }

    if ((posX < m_config.ai.deadzoneBoundary && action.moveRight) ||
        (posX > m_config.windowWidth - m_config.ai.deadzoneBoundary &&
         action.moveLeft)) {
      reward += m_config.ai.escapeDeadzoneReward;
    }
  }

  if (action.type == ActionType::Noop) {

    if (state.distanceToOpponent < m_config.ai.optimalDistance * 0.8f ||
        state.myHealth < state.opponentHealth) {
      reward -= 5.0f;
    }

    if (!m_actionHistory.empty() &&
        m_actionHistory.back() == ActionType::Noop) {
      reward -= 10.0f;
    }
  }

  float distanceToOpponent = state.distanceToOpponent;
  float distanceScore =
      -std::abs(distanceToOpponent - m_config.ai.optimalDistance) *
      m_config.ai.distanceMultiplier;
  reward += distanceScore;

  if (action.attack) {
    if (m_character->lastAttackLanded) {
      reward += m_config.ai.hitReward;

      if (!m_actionHistory.empty() && m_actionHistory.back() == action.type) {
        float comboMultiplier =
            std::min(m_config.ai.maxComboMultiplier,
                     1.0f + (m_comboCount * m_config.ai.comboBaseMultiplier));
        reward += m_config.ai.hitReward * comboMultiplier;
      }

      if (std::abs(distanceToOpponent - m_config.ai.optimalDistance) < 50.0f) {
        reward += m_config.ai.optimalDistanceBonus;
      }
    } else {
      reward += m_config.ai.missPenalty;

      if (distanceToOpponent > m_config.ai.optimalDistance * 1.5f) {
        reward += m_config.ai.farWhiffPenalty;
      }
    }
  }

  if (action.block) {
    if (m_character->lastBlockEffective) {
      reward += m_config.ai.blockReward;
      if (state.opponentLastActions[0] == ActionType::Attack) {
        reward += m_config.ai.wellTimedBlockBonus;
      }
    } else {
      reward += m_config.ai.blockPenalty;
    }
  }

  if (m_character->stamina <= 0.0f) {
    reward += m_config.ai.noStaminaPenalty;
  } else if (m_character->stamina <
             m_character->maxStamina * m_config.ai.lowStaminaThreshold) {
    reward += m_config.ai.lowStaminaPenalty;
  }

  if (m_actionHistory.size() >= 3) {
    bool sameAction =
        std::all_of(m_actionHistory.begin(), m_actionHistory.end(),
                    [first = m_actionHistory.front()](const ActionType &act) {
                      return act == first;
                    });
    if (sameAction) {
      reward += m_config.ai.repeatActionPenalty;
    }
  }

  reward -= m_battleStyle.timePenalty;
  reward += healthDiff * m_battleStyle.hpRatioWeight;
  reward -= std::abs(distanceToOpponent - m_config.ai.optimalDistance) *
            m_battleStyle.distancePenalty;

  return reward;
}

bool RLAgent::isPassiveNoOp(const Experience &exp) {
  float hpChange = std::abs(exp.nextState.myHealth - exp.state.myHealth);
  float distChange =
      std::abs(exp.nextState.distanceToOpponent - exp.state.distanceToOpponent);
  const float threshold = 0.01f;
  return (exp.action.type == ActionType::Noop && hpChange < threshold &&
          distChange < threshold);
}

void RLAgent::updateReplayBuffer(const Experience &exp) {
  if (isPassiveNoOp(exp))
    return;

  auto s = stateToVector(exp.state);
  auto s_next = stateToVector(exp.nextState);
  auto current_q = onlineDQN->forward(s);
  auto next_q = targetDQN->forward(s_next);
  float max_next_q = *std::max_element(next_q.begin(), next_q.end());
  int action_index = static_cast<int>(exp.action.type);
  float td_error = std::abs(exp.reward + m_discountFactor * max_next_q -
                            current_q[action_index]);

  if (exp.action.type != ActionType::Noop) {
    td_error *= 1.2f;
  }

  PrioritizedExperience pe{exp, td_error};
  if (replayBuffer.size() >= MAX_REPLAY_BUFFER)
    replayBuffer.pop();
  replayBuffer.push(pe);
}

void RLAgent::learn(const Experience &exp) {
  updateReplayBuffer(exp);
  sampleAndTrain();
}

void RLAgent::applyAction(const Action &action) {
  std::string currentAnim = m_character->animator->getCurrentAnimationKey();
  bool isAttackingOrBlocking =
      (currentAnim == "Attack" || currentAnim == "Attack 2" ||
       currentAnim == "Attack 3" || currentAnim == "Block");

  float moveForce = m_config.moveForce;
  if (!isAttackingOrBlocking) {
    if (action.moveLeft) {
      m_character->mover.applyForce(Vector2f(-moveForce, 0));
      m_character->isMoving = true;
      m_character->inputDirection = -1;
    } else if (action.moveRight) {
      m_character->mover.applyForce(Vector2f(moveForce, 0));
      m_character->isMoving = true;
      m_character->inputDirection = 1;
    } else {
      m_character->isMoving = false;
    }
  }
  if (action.jump && m_character->onGround)
    m_character->jump();
  if (action.attack)
    m_character->attack();
  if (action.block)
    m_character->block();
}

void RLAgent::updateStance(const State &state) {
  if (state.myHealth < 0.3f)
    m_currentStance = Stance::Defensive;
  else if (state.opponentHealth < 0.4f)
    m_currentStance = Stance::Aggressive;
  else
    m_currentStance = Stance::Neutral;
}

Action RLAgent::predictOpponentAction(const State &state) {
  if (m_opponentActionHistory.empty()) {
    int random_action = static_cast<int>(m_dist(m_gen) * num_actions);
    return Action::fromType(static_cast<ActionType>(random_action));
  }

  std::map<ActionType, int> freq;
  for (auto act : m_opponentActionHistory) {
    freq[act]++;
  }
  ActionType mostCommon = m_opponentActionHistory.front();
  int maxCount = 0;
  for (auto &p : freq) {
    if (p.second > maxCount) {
      maxCount = p.second;
      mostCommon = p.first;
    }
  }
  return Action::fromType(mostCommon);
}

void RLAgent::incrementEpisodeCount() { m_episodeCount++; }

void RLAgent::reportWin(bool didWin) {
  m_totalRounds++;
  if (didWin) {
    m_wins++;
  }

  m_winRate =
      m_totalRounds > 0 ? static_cast<float>(m_wins) / m_totalRounds : 0.0f;

  Logger::debug("Round ended - Wins: %d/%d (%.2f%%)", m_wins, m_totalRounds,
                m_winRate * 100.0f);
}

void RLAgent::updateTargetNetwork() {

  const auto &onlineLayers = onlineDQN->getLayers();
  for (size_t i = 0; i < onlineLayers.size(); ++i) {
    targetDQN->setLayerParameters(i, onlineLayers[i].weights,
                                  onlineLayers[i].biases);
  }
  Logger::debug("Target network updated");
}

void RLAgent::trackActionHistory(ActionType action, bool isOpponent) {
  if (isOpponent) {
    m_opponentActionHistory.push_back(action);
    if (m_opponentActionHistory.size() > 10)
      m_opponentActionHistory.pop_front();
  } else {
    m_actionHistory.push_back(action);
    if (m_actionHistory.size() > 10)
      m_actionHistory.pop_front();
  }
}

void RLAgent::updateComboSystem(const Action &action) {
  if (action.attack) {
    if (m_character->lastAttackLanded) {
      m_comboCount++;
      m_totalReward += 5.0f * m_comboCount;
    } else {
      m_comboCount = 0;
    }

    m_character->comboCount = m_comboCount;
  }
}

void RLAgent::update(float deltaTime, const Character &opponent) {
  if (m_episodeCount > 0)
    decayEpsilon();

  if (m_moveHoldCounter > 0) {
    m_moveHoldCounter--;
    applyAction(m_lastAction);
    return;
  }

  m_opponentVelocity = opponent.mover.position - m_lastOpponentPosition;
  m_lastOpponentPosition = opponent.mover.position;

  {
    std::string oppAnim = opponent.animator->getCurrentAnimationKey();
    ActionType oppAction = animationKeyToActionType(oppAnim);
    trackActionHistory(oppAction, true);
  }

  m_episodeTime += deltaTime;
  m_timeSinceLastAction += deltaTime;
  m_currentActionDuration += deltaTime;

  State newState = getCurrentState(opponent);
  updateStance(newState);
  if (m_currentActionDuration >= m_actionHoldDuration) {
    Action newAction = selectAction(newState);
    if ((newAction.moveLeft && m_lastAction.moveLeft) ||
        (newAction.moveRight && m_lastAction.moveRight)) {
      m_moveHoldCounter = MOVE_HOLD_TICKS - 1;
      m_actionHoldDuration = 0.5f;
    } else {
      m_actionHoldDuration = 0.3f;
    }
    trackActionHistory(m_lastAction.type, false);

    float healthDiff = m_character->health - m_lastHealth;
    if (healthDiff != 0) {
      if (healthDiff < 0)
        m_consecutiveWhiffs++;
      else
        m_consecutiveWhiffs = 0;
      m_timeSinceLastAction = 0;
    }
    m_lastHealth = m_character->health;
    float reward = calculateReward(newState, newAction);
    m_totalReward += reward;
    Experience exp{m_currentState, m_lastAction, reward, newState};
    learn(exp);
    m_currentState = newState;
    m_lastAction = newAction;
    m_currentActionDuration = 0;
    updateComboSystem(newAction);
    Logger::debug("Selected action: %s", actionTypeToString(m_lastAction.type));
  }
  applyAction(m_lastAction);
}

void RLAgent::reset() {
  m_totalReward = 0;
  m_episodeTime = 0;
  m_currentState = getCurrentState(*m_character);
  m_lastAction = Action::fromType(ActionType::Noop);
  while (!replayBuffer.empty())
    replayBuffer.pop();
  m_moveHoldCounter = 0;
  m_comboCount = 0;
  m_actionHistory.clear();
  m_opponentActionHistory.clear();
}

void RLAgent::startNewEpoch() {
  bool didWin = (m_character->health > (m_character->maxHealth * 0.5f));
  reportWin(didWin);

  m_character->mover.position = {100, 100};
  m_character->health = m_character->maxHealth;
  reset();
  Logger::info("Starting new epoch, reward reset.");
}

float RLAgent::calculatePriority(float td_error) const {

  return std::pow(std::abs(td_error) + PRIORITY_EPSILON, m_per_alpha);
}

float RLAgent::calculateImportanceWeight(float priority,
                                         float max_priority) const {

  float normalized_priority = priority / max_priority;
  return std::pow(normalized_priority, -m_per_beta);
}

void RLAgent::softUpdateTargetNetwork() {
  const auto &online_layers = onlineDQN->getLayers();
  const auto &target_layers = targetDQN->getLayers();

  for (size_t i = 0; i < online_layers.size(); ++i) {
    std::vector<std::vector<float>> new_weights = target_layers[i].weights;
    std::vector<float> new_biases = target_layers[i].biases;

    for (size_t j = 0; j < new_weights.size(); ++j) {
      for (size_t k = 0; k < new_weights[j].size(); ++k) {
        new_weights[j][k] = m_tau * online_layers[i].weights[j][k] +
                            (1 - m_tau) * target_layers[i].weights[j][k];
      }
      new_biases[j] = m_tau * online_layers[i].biases[j] +
                      (1 - m_tau) * target_layers[i].biases[j];
    }

    targetDQN->setLayerParameters(i, new_weights, new_biases);
  }
}

std::vector<float> RLAgent::getActionMask(const State &state) const {
  std::vector<float> mask(num_actions, 1.0f);

  if (state.isCornered) {
    float posX = m_character->mover.position.x;
    if (posX < m_config.ai.deadzoneBoundary) {

      mask[static_cast<int>(ActionType::MoveLeft)] = 0.0f;
      mask[static_cast<int>(ActionType::MoveLeftAttack)] = 0.0f;
    } else if (posX > m_config.windowWidth - m_config.ai.deadzoneBoundary) {

      mask[static_cast<int>(ActionType::MoveRight)] = 0.0f;
      mask[static_cast<int>(ActionType::MoveRightAttack)] = 0.0f;
    }
  }

  if (state.myStamina < 0.2f) {
    mask[static_cast<int>(ActionType::Attack)] = 0.0f;
    mask[static_cast<int>(ActionType::JumpAttack)] = 0.0f;
    mask[static_cast<int>(ActionType::MoveLeftAttack)] = 0.0f;
    mask[static_cast<int>(ActionType::MoveRightAttack)] = 0.0f;
  }

  return mask;
}

void RLAgent::sampleAndTrain() {
  if (replayBuffer.size() < MIN_EXPERIENCES_BEFORE_TRAINING) {
    return;
  }

  std::vector<Experience> batch;
  std::vector<float> priorities;
  std::vector<float> weights;
  float max_priority = replayBuffer.top().priority;

  for (int i = 0; i < BATCH_SIZE && !replayBuffer.empty(); ++i) {
    auto exp = replayBuffer.top();
    batch.push_back(exp.exp);
    priorities.push_back(exp.priority);
    weights.push_back(calculateImportanceWeight(exp.priority, max_priority));
    replayBuffer.pop();
  }

  float max_weight = *std::max_element(weights.begin(), weights.end());
  for (auto &w : weights) {
    w /= max_weight;
  }

  for (size_t i = 0; i < batch.size(); ++i) {
    auto &experience = batch[i];

    auto current_state = stateToVector(experience.state);
    auto next_state = stateToVector(experience.nextState);

    auto current_mask = getActionMask(experience.state);
    auto next_mask = getActionMask(experience.nextState);

    auto current_q = onlineDQN->forward(current_state);
    auto next_q = targetDQN->forward(next_state);
    auto online_next_q = onlineDQN->forward(next_state);

    for (size_t j = 0; j < current_q.size(); ++j) {
      current_q[j] *= current_mask[j];
      next_q[j] *= next_mask[j];
      online_next_q[j] *= next_mask[j];
    }

    int best_action =
        std::max_element(online_next_q.begin(), online_next_q.end()) -
        online_next_q.begin();
    float next_q_value = next_q[best_action];

    float scaled_reward = experience.reward * m_reward_scale;
    float target = scaled_reward + m_gamma * next_q_value;

    int action_index = static_cast<int>(experience.action.type);
    current_q[action_index] = target;

    float effective_lr = m_learningRate * weights[i];
    onlineDQN->train(current_state, current_q, effective_lr);
  }

  softUpdateTargetNetwork();
}

void RLAgent::decayEpsilon() {
  m_epsilon = std::max(m_epsilon_min, m_epsilon * m_epsilon_decay);

  m_per_beta = std::min(1.0f, m_per_beta + 0.001f);
}
