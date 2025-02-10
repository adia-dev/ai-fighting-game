#include "RLAgent.hpp"
#include "Core/Logger.hpp"
#include <algorithm>
#include <cmath>

static constexpr float DEFAULT_EPISODE_DURATION = 60.0f;
static constexpr int TARGET_UPDATE_FREQUENCY = 1000;

RLAgent::RLAgent(Character *character)
    : m_character(character), m_totalReward(0), m_episodeTime(0),
      m_episodeDuration(DEFAULT_EPISODE_DURATION), m_timeSinceLastAction(0),
      m_lastHealth(100), m_currentActionDuration(0), m_actionHoldDuration(0.2f),
      m_consecutiveWhiffs(0), m_lastOpponentHealth(0), m_epsilon(0.3f),
      m_learningRate(0.001f), m_discountFactor(0.95f), m_episodeCount(0),
      updateCounter(0), m_wins(0), m_gen(m_rd()), m_dist(0.0f, 1.0f),
      m_moveHoldCounter(0), m_currentStance(Stance::Neutral), m_comboCount(0) {

  state_dim = 14;
  num_actions = 9;

  onlineDQN = std::make_unique<NeuralNetwork>(state_dim);
  onlineDQN->addLayer(64, ActivationType::ReLU);
  onlineDQN->addLayer(128, ActivationType::ReLU);
  onlineDQN->addLayer(num_actions, ActivationType::None);

  targetDQN = std::make_unique<NeuralNetwork>(state_dim);
  targetDQN->addLayer(64, ActivationType::ReLU);
  targetDQN->addLayer(num_actions, ActivationType::None);

  m_battleStyle.timePenalty = 0.004f;
  m_battleStyle.hpRatioWeight = 1.0f;
  m_battleStyle.distancePenalty = 0.0002f;

  m_lastOpponentPosition = m_character->mover.position;
  m_opponentVelocity = Vector2f(0, 0);

  reset();
}

std::vector<float> RLAgent::stateToVector(const State &state) {
  std::vector<float> v;
  v.push_back(state.distanceToOpponent);
  v.push_back(state.relativePositionX);
  v.push_back(state.relativePositionY);
  v.push_back(state.myHealth);
  v.push_back(state.opponentHealth);
  v.push_back(state.timeSinceLastAction);
  for (int i = 0; i < 4; ++i)
    v.push_back(state.radar[i]);
  v.push_back(state.opponentVelocityX);
  v.push_back(state.opponentVelocityY);
  v.push_back(state.isCornered ? 1.0f : 0.0f);

  for (auto action : state.lastActions)
    v.push_back(static_cast<float>(action));
  for (auto action : state.opponentLastActions)
    v.push_back(static_cast<float>(action));
  v.push_back(state.predictedDistance);
  v.push_back(static_cast<float>(state.currentStance));
  return v;
}

State RLAgent::getCurrentState(const Character &opponent) {
  State state;
  Vector2f toOpponent = opponent.mover.position - m_character->mover.position;
  state.distanceToOpponent = toOpponent.length();
  state.relativePositionX = toOpponent.x;
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
  state.isCornered = (posX < 150 || posX > 850);

  std::array<ActionType, 3> selfHist = {ActionType::Noop, ActionType::Noop,
                                        ActionType::Noop};
  std::array<ActionType, 3> oppHist = {ActionType::Noop, ActionType::Noop,
                                       ActionType::Noop};
  int i = 0;
  for (auto it = m_actionHistory.rbegin();
       it != m_actionHistory.rend() && i < 3; ++it, ++i)
    selfHist[i] = *it;
  i = 0;
  for (auto it = m_opponentActionHistory.rbegin();
       it != m_opponentActionHistory.rend() && i < 3; ++it, ++i)
    oppHist[i] = *it;
  state.lastActions = selfHist;
  state.opponentLastActions = oppHist;

  Vector2f predictedPos = opponent.mover.position + m_opponentVelocity * 0.5f;
  state.predictedDistance =
      (predictedPos - m_character->mover.position).length();

  state.currentStance = m_currentStance;
  return state;
}

Action RLAgent::selectAction(const State &state) {
  auto state_vec = stateToVector(state);
  auto q_values = onlineDQN->forward(state_vec);
  Action selectedAction;
  if (m_dist(m_gen) < m_epsilon) {
    int random_action = static_cast<int>(m_dist(m_gen) * num_actions);
    selectedAction = Action::fromType(static_cast<ActionType>(random_action));
  } else {
    int bestAction = std::distance(
        q_values.begin(), std::max_element(q_values.begin(), q_values.end()));
    selectedAction = Action::fromType(static_cast<ActionType>(bestAction));
  }

  Action predictedOppAction = predictOpponentAction(state);
  if (predictedOppAction.type == ActionType::Attack &&
      selectedAction.type != ActionType::Block)
    selectedAction = Action::fromType(ActionType::Block);

  if (m_currentStance == Stance::Defensive &&
      (selectedAction.type == ActionType::Attack ||
       selectedAction.type == ActionType::JumpAttack))
    selectedAction = Action::fromType(ActionType::Block);

  return selectedAction;
}

float RLAgent::calculateReward(const State &state, const Action &action) {
  float reward = 0.0f;

  reward += (state.myHealth - state.opponentHealth) * 15.0f;

  if (action.attack && m_character->lastAttackLanded) {
    reward += 25.0f;
    if (!m_actionHistory.empty() && m_actionHistory.back() == action.type)
      reward += 10.0f * (++m_comboCount);
  } else if (action.attack) {
    reward -= 8.0f;
    m_comboCount = 0;
  }

  const float stageWidth = 1000.0f;
  float posX = m_character->mover.position.x;
  if (posX < 150 || posX > stageWidth - 150)
    reward -= 15.0f;
  float oppX = m_lastOpponentPosition.x;
  if (oppX < 150 || oppX > stageWidth - 150)
    reward += 10.0f;

  float approachSpeed = state.opponentVelocityX *
                        (state.relativePositionX / state.distanceToOpponent);
  reward += approachSpeed * 0.2f;

  if (action.block && m_character->lastBlockEffective)
    reward += 15.0f;
  else if (action.block)
    reward -= 5.0f;

  if (m_actionHistory.size() >= 3) {
    bool same = true;
    auto it = m_actionHistory.end();
    ActionType last = *(--it);
    for (int i = 0; i < 2; i++) {
      if (*(--it) != last) {
        same = false;
        break;
      }
    }
    if (same)
      reward -= 7.0f;
  }

  reward -= m_battleStyle.timePenalty;
  reward +=
      (state.myHealth - state.opponentHealth) * m_battleStyle.hpRatioWeight;
  reward -= std::abs(state.distanceToOpponent - 150.0f) *
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
  PrioritizedExperience pe{exp, td_error};
  if (replayBuffer.size() >= MAX_REPLAY_BUFFER)
    replayBuffer.pop();
  replayBuffer.push(pe);
}

void RLAgent::sampleAndTrain() {
  if (replayBuffer.size() < BATCH_SIZE)
    return;
  std::vector<PrioritizedExperience> batch;
  for (size_t i = 0; i < BATCH_SIZE; ++i) {
    auto pe = replayBuffer.top();
    replayBuffer.pop();
    batch.push_back(pe);
  }
  for (auto &pe : batch) {
    auto s = stateToVector(pe.exp.state);
    auto s_next = stateToVector(pe.exp.nextState);
    auto current_q = onlineDQN->forward(s);
    auto next_q = targetDQN->forward(s_next);
    float max_next_q = *std::max_element(next_q.begin(), next_q.end());
    int action_index = static_cast<int>(pe.exp.action.type);
    current_q[action_index] = pe.exp.reward + m_discountFactor * max_next_q;
    onlineDQN->train(s, current_q, m_learningRate);
  }
  updateCounter += BATCH_SIZE;
  if (updateCounter >= TARGET_UPDATE_FREQUENCY) {
    targetDQN = std::make_unique<NeuralNetwork>(*onlineDQN);
    updateCounter = 0;
  }
}

void RLAgent::learn(const Experience &exp) {
  updateReplayBuffer(exp);
  sampleAndTrain();
}

void RLAgent::applyAction(const Action &action) {
  const float MOVE_FORCE = 500.0f;
  if (action.moveLeft)
    m_character->mover.applyForce(Vector2f(-MOVE_FORCE, 0));
  if (action.moveRight)
    m_character->mover.applyForce(Vector2f(MOVE_FORCE, 0));
  if (action.jump && m_character->onGround)
    m_character->jump();
  if (action.attack)
    m_character->attack();
  if (action.block)
    m_character->block();
}

void RLAgent::decayEpsilon() {
  m_epsilon = std::max(0.01f, m_epsilon * 0.995f);
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

  int random_action = static_cast<int>(m_dist(m_gen) * num_actions);
  return Action::fromType(static_cast<ActionType>(random_action));
}

void RLAgent::trackActionHistory(ActionType action, bool isOpponent) {
  if (isOpponent) {
    m_opponentActionHistory.push_back(action);
    if (m_opponentActionHistory.size() > 3)
      m_opponentActionHistory.pop_front();
  } else {
    m_actionHistory.push_back(action);
    if (m_actionHistory.size() > 3)
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
  m_character->mover.position = {100, 100};
  m_character->health = m_character->maxHealth;
  reset();
  Logger::info("Starting new epoch, reward reset.");
}
