#include "RLAgent.hpp"
#include "AI/RLVisualization.hpp"
#include "Core/Logger.hpp"

static constexpr float DEFAULT_EPISODE_DURATION = 60.0f;

RLAgent::RLAgent(Character *character)
    : m_character(character), m_totalReward(0), m_episodeTime(0),
      m_episodeDuration(DEFAULT_EPISODE_DURATION), m_timeSinceLastAction(0),
      m_timeSinceLastDamage(0), m_lastHealth(100), m_epsilon(0.3f),
      m_learningRate(0.1f), m_discountFactor(0.95f), m_episodeCount(0),
      m_gen(m_rd()), m_dist(0.0f, 1.0f) {
  initQTable();
  reset();
}

void RLAgent::update(float deltaTime, const Character &opponent) {
  m_episodeTime += deltaTime;
  // Update timers
  m_timeSinceLastAction += deltaTime;
  m_timeSinceLastDamage += deltaTime;
  m_currentActionDuration += deltaTime;

  // Get current state
  State newState = getCurrentState(opponent);

  // Only make new decisions if we've completed our current action duration
  if (m_currentActionDuration >= m_actionHoldDuration) {
    // Select action using epsilon-greedy policy
    Action action = selectAction(newState);

    // Track damage dealt/received
    float healthDiff = m_character->health - m_lastHealth;
    if (healthDiff != 0) {
      if (healthDiff < 0) {
        m_consecutiveWhiffs++;
        Logger::debug("Took damage: " + std::to_string(-healthDiff));
      } else {
        m_consecutiveWhiffs = 0;
      }
      m_timeSinceLastDamage = 0;
    }
    m_lastHealth = m_character->health;

    // Calculate reward
    float reward = calculateReward(newState, action);
    m_totalReward += reward;

    // Store experience and learn
    Experience exp{m_currentState, m_lastAction, reward, newState, false};
    learn(exp);

    // Update state and action
    m_currentState = newState;
    m_lastAction = action;

    // Reset action duration for new action
    m_currentActionDuration = 0;
    m_actionHoldDuration = calculateActionDuration(action);

    logDecision(newState, action, reward);
  }

  // Apply current action
  applyAction(m_lastAction);

  // Update visualization
  updateRadar();
  updateStateDisplay();
}

float RLAgent::calculateActionDuration(const Action &action) {
  // Hold movement actions longer for smoother motion
  if (action.moveLeft || action.moveRight) {
    return 0.2f + m_dist(m_gen) * 0.1f; // 0.2-0.3 seconds
  }

  // Jump actions need time to complete
  if (action.jump) {
    return 0.4f;
  }

  // Attack actions should complete their animation
  if (action.attack) {
    return 0.3f;
  }

  return 0.1f; // Default duration
}

void RLAgent::updateStats() {
  m_stats.totalEpisodes = m_episodeCount;
  if (!m_stats.rewardHistory.empty()) {
    float totalReward = 0;
    for (float reward : m_stats.rewardHistory) {
      totalReward += reward;
    }
    m_stats.avgReward = totalReward / m_stats.rewardHistory.size();
    m_stats.bestReward = *std::max_element(m_stats.rewardHistory.begin(),
                                           m_stats.rewardHistory.end());
  }
}

State RLAgent::getCurrentState(const Character &opponent) {
  Vector2f toOpponent = opponent.mover.position - m_character->mover.position;

  State state;
  state.distanceToOpponent = toOpponent.length();
  state.relativePositionX = toOpponent.x;
  state.relativePositionY = toOpponent.y;
  state.myHealth =
      static_cast<float>(m_character->health) / m_character->maxHealth;
  state.opponentHealth =
      static_cast<float>(opponent.health) / opponent.maxHealth;
  // TODO: Implement stamina system
  // state.myStamina = 1.0f;
  state.amIOnGround = m_character->onGround;
  state.isOpponentOnGround = opponent.onGround;
  state.myCurrentAnimation = m_character->animator->getCurrentAnimationKey();
  state.opponentCurrentAnimation = opponent.animator->getCurrentAnimationKey();
  state.timeSinceLastAction = m_timeSinceLastAction;

  return state;
}

Action RLAgent::selectAction(const State &state) {
  StateKey key = state.toKey();
  std::string stateStr = key.toString();

  // Increase exploration when performing poorly
  if (state.myHealth < state.opponentHealth) {
    m_epsilon = std::min(m_epsilon * 1.1f, 0.4f);
  }

  // Epsilon-greedy selection with context
  if (m_dist(m_gen) < m_epsilon) {
    // Biased random selection based on state
    std::vector<ActionType> preferredActions;

    if (state.opponentCurrentAnimation.find("Attack") != std::string::npos) {
      // Prefer defensive actions when opponent is attacking
      preferredActions = {ActionType::MoveLeft, ActionType::MoveRight,
                          ActionType::Jump};
    } else if (state.distanceToOpponent < 150.0f) {
      // Prefer attacks at close range
      preferredActions = {ActionType::Attack, ActionType::MoveLeftAttack,
                          ActionType::MoveRightAttack};
    } else if (state.distanceToOpponent > 300.0f) {
      // Prefer movement when far
      preferredActions = {ActionType::MoveLeft, ActionType::MoveRight};
    }

    if (!preferredActions.empty() && m_dist(m_gen) < 0.7f) {
      return Action::fromType(preferredActions[static_cast<int>(
          m_dist(m_gen) * preferredActions.size())]);
    }

    // Fall back to completely random action
    return Action::fromType(
        static_cast<ActionType>(static_cast<int>(m_dist(m_gen) * 8)));
  }

  // Select best action from Q-table
  ActionType bestAction = ActionType::None;
  float bestValue = -std::numeric_limits<float>::max();

  for (const auto &pair : m_qTable[stateStr]) {
    if (pair.second > bestValue) {
      bestValue = pair.second;
      bestAction = pair.first;
    }
  }

  return Action::fromType(bestAction);
}

float RLAgent::calculateReward(const State &state, const Action &action) {
  float reward = 0.0f;

  // Base combat rewards
  if (action.attack) {
    if (state.distanceToOpponent < 150.0f) {
      // Reward for attacking at proper range
      reward += 5.0f;

      // Extra reward for attacking vulnerable opponent
      if (state.isOpponentVulnerable) {
        reward += 15.0f;
      }

      // Combo rewards
      if (state.myComboCounter > 0) {
        reward += state.myComboCounter * 3.0f;
      }
    } else {
      // Punish whiffing attacks
      reward -= 3.0f;
      m_consecutiveWhiffs++;
    }
  }

  // Positioning rewards
  float optimalDistance = 120.0f;
  if (state.opponentCurrentAnimation.find("Attack") != std::string::npos) {
    // Stay further when opponent is attacking
    optimalDistance = 180.0f;
  }
  float distanceError = std::abs(state.distanceToOpponent - optimalDistance);
  reward -= distanceError * 0.01f;

  // Defensive rewards
  if (state.opponentCurrentAnimation.find("Attack") != std::string::npos) {
    if (action.moveLeft || action.moveRight) {
      reward += 4.0f; // Reward evasive movement
    }
    if (action.jump && state.distanceToOpponent < 150.0f) {
      reward += 3.0f; // Reward defensive jumps
    }
  }

  // Wall positioning
  if (state.distanceToWall < 50.0f) {
    reward -= 2.0f; // Punish being cornered
  }

  // Health-based rewards
  float healthDiff = state.myHealth - state.opponentHealth;
  reward += healthDiff * 15.0f;

  // Time management
  if (healthDiff > 0 && m_episodeTime > 45.0f) {
    reward += 1.0f; // Reward maintaining health lead late in round
  }

  // Punish excessive whiffing
  if (m_consecutiveWhiffs > 2) {
    reward -= m_consecutiveWhiffs * 8.0f;
  }

  return reward;
}

void RLAgent::learn(const Experience &exp) {
  StateKey currentKey = exp.state.toKey();
  StateKey nextKey = exp.nextState.toKey();

  // Get current Q value
  float currentQ = m_qTable[currentKey.toString()][exp.action.type];

  // Find max Q value for next state
  float maxNextQ = -std::numeric_limits<float>::max();
  for (const auto &pair : m_qTable[nextKey.toString()]) {
    maxNextQ = std::max(maxNextQ, pair.second);
  }

  // Q-learning update
  float newQ =
      currentQ +
      m_learningRate * (exp.reward + m_discountFactor * maxNextQ - currentQ);

  m_qTable[currentKey.toString()][exp.action.type] = newQ;

  Logger::debug("Learning from experience - Reward: " +
                std::to_string(exp.reward));
}

void RLAgent::updateRadar() {
  m_radarBlips.clear();

  // Add opponent position blip
  RadarBlip opponentBlip;
  opponentBlip.position = Vector2f(m_currentState.relativePositionX, 0) +
                          Vector2f(0, m_currentState.relativePositionY);
  opponentBlip.intensity = 1.0f;
  m_radarBlips.push_back(opponentBlip);
}

void RLAgent::render(SDL_Renderer *renderer) {
  // Radar
  SDL_Rect radarBounds = {10, 10, 150, 150};
  RLVisualization::renderRadar(renderer,
                               Vector2f(m_currentState.relativePositionX,
                                        m_currentState.relativePositionY),
                               m_currentState.distanceToOpponent, radarBounds);

  // State panel
  SDL_Rect stateBounds = {10, 170, 150, 150};
  RLVisualization::renderStatePanel(
      renderer, stateBounds, m_currentState.myHealth,
      m_currentState.opponentHealth, actionTypeToString(m_lastAction.type),
      m_totalReward, getLastActionConfidence());
}

void RLAgent::renderRadar(SDL_Renderer *renderer) {
  // Draw radar background
  SDL_SetRenderDrawColor(renderer, 0, 50, 0, 100);
  SDL_Rect radarRect = {10, 10, 200, 200};
  SDL_RenderFillRect(renderer, &radarRect);

  // Draw radar grid
  SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
  for (int i = 0; i <= 4; ++i) {
    int x = radarRect.x + (radarRect.w * i) / 4;
    int y = radarRect.y + (radarRect.h * i) / 4;
    SDL_RenderDrawLine(renderer, x, radarRect.y, x, radarRect.y + radarRect.h);
    SDL_RenderDrawLine(renderer, radarRect.x, y, radarRect.x + radarRect.w, y);
  }

  // Draw radar blips
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  for (const auto &blip : m_radarBlips) {
    int x = radarRect.x + radarRect.w / 2 +
            static_cast<int>(blip.position.x * 0.1f);
    int y = radarRect.y + radarRect.h / 2 +
            static_cast<int>(blip.position.y * 0.1f);
    SDL_Rect blipRect = {x - 2, y - 2, 5, 5};
    SDL_RenderFillRect(renderer, &blipRect);
  }
}

void RLAgent::renderDebugInfo(SDL_Renderer *renderer) {
  // Draw debug text
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_Rect debugRect = {220, 10, 200, 200};
  SDL_RenderDrawRect(renderer, &debugRect);

  // TODO: Add text rendering for debug info:
  // - Current state values
  // - Selected action
  // - Current reward
  // - Episode statistics
}

void RLAgent::logDecision(const State &state, const Action &action,
                          float reward) {
  std::string actionStr = std::string(action.moveLeft ? "Left " : "") +
                          (action.moveRight ? "Right " : "") +
                          (action.jump ? "Jump " : "") +
                          (action.attack ? "Attack" : "");

  Logger::debug("RL Decision - Action: " + actionStr +
                " Reward: " + std::to_string(reward) +
                " Distance: " + std::to_string(state.distanceToOpponent));
}

void RLAgent::reset() {
  m_totalReward = 0;
  m_episodeTime = 0;
  m_timeSinceLastAction = 0;
  m_replayBuffer.clear();
}

void RLAgent::startNewEpoch() {
  // Reset positions
  m_character->mover.position = Vector2f(100, 100);

  // Reset health
  m_character->health = m_character->maxHealth;

  // Reset state
  reset();

  Logger::info("Starting new epoch - Total reward from previous epoch: " +
               std::to_string(m_totalReward));
}

void RLAgent::initQTable() {
  std::vector<ActionType> allActions = {
      ActionType::None,           ActionType::MoveLeft,
      ActionType::MoveRight,      ActionType::Jump,
      ActionType::Attack,         ActionType::JumpAttack,
      ActionType::MoveLeftAttack, ActionType::MoveRightAttack};

  // Initialize Q-values to small random values
  for (const auto &action : allActions) {
    for (int dist = 0; dist < 5; dist++) {
      for (int health = -2; health <= 2; health++) {
        for (int attacking = 0; attacking <= 1; attacking++) {
          for (int oppAttacking = 0; oppAttacking <= 1; oppAttacking++) {
            for (int onGround = 0; onGround <= 1; onGround++) {
              StateKey key{dist, health, static_cast<bool>(attacking),
                           static_cast<bool>(oppAttacking),
                           static_cast<bool>(onGround)};
              m_qTable[key.toString()][action] = m_dist(m_gen) * 0.1f;
            }
          }
        }
      }
    }
  }
}
void RLAgent::renderStateDisplay(SDL_Renderer *renderer) {
  // Draw state info panel
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
  SDL_Rect stateRect = {10, 400, 200, 180};
  SDL_RenderFillRect(renderer, &stateRect);

  // Draw decision confidence bar
  float confidence = getLastActionConfidence();
  SDL_SetRenderDrawColor(renderer,
                         static_cast<Uint8>(255 * (1.0f - confidence)),
                         static_cast<Uint8>(255 * confidence), 0, 255);
  SDL_Rect confRect = {stateRect.x + 5, stateRect.y + 5,
                       static_cast<int>(190 * confidence), 20};
  SDL_RenderFillRect(renderer, &confRect);

  // Draw state value indicators
  renderStateValues(renderer, stateRect);

  // Draw action intention indicators
  renderActionIntentions(renderer, stateRect);
}

void RLAgent::renderStateValues(SDL_Renderer *renderer,
                                const SDL_Rect &bounds) {
  const int BAR_HEIGHT = 15;
  const int BAR_SPACING = 20;
  int y = bounds.y + 30;

  // Distance gauge
  float distanceRatio = m_currentState.distanceToOpponent / 400.0f;
  drawHorizontalGauge(renderer, bounds.x + 5, y, 190, BAR_HEIGHT, distanceRatio,
                      {0, 255, 255, 255});
  y += BAR_SPACING;

  // Health advantage gauge
  float healthAdvantage =
      (m_currentState.myHealth - m_currentState.opponentHealth + 1.0f) * 0.5f;
  drawHorizontalGauge(renderer, bounds.x + 5, y, 190, BAR_HEIGHT,
                      healthAdvantage, {255, 255, 0, 255});
  y += BAR_SPACING;

  // Attack readiness gauge
  float attackReadiness = 1.0f - (m_timeSinceLastAction / 2.0f);
  drawHorizontalGauge(renderer, bounds.x + 5, y, 190, BAR_HEIGHT,
                      attackReadiness, {255, 0, 0, 255});
}

void RLAgent::drawHorizontalGauge(SDL_Renderer *renderer, int x, int y,
                                  int width, int height, float value,
                                  SDL_Color color) {
  // Background
  SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
  SDL_Rect bgRect = {x, y, width, height};
  SDL_RenderFillRect(renderer, &bgRect);

  // Value bar
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_Rect valueRect = {x, y, static_cast<int>(width * value), height};
  SDL_RenderFillRect(renderer, &valueRect);

  // Border
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderDrawRect(renderer, &bgRect);
}

void RLAgent::renderActionIntentions(SDL_Renderer *renderer,
                                     const SDL_Rect &bounds) {
  int y = bounds.y + 100;
  const int ARROW_SIZE = 20;

  // Movement intentions
  if (m_lastAction.moveLeft || m_lastAction.moveRight) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    drawArrow(renderer, bounds.x + bounds.w / 2, y,
              bounds.x + bounds.w / 2 +
                  (m_lastAction.moveRight ? ARROW_SIZE : -ARROW_SIZE),
              y, 5);
  }

  // Attack intention
  if (m_lastAction.attack) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    const SDL_Point points[3] = {
        {bounds.x + bounds.w / 2, y + ARROW_SIZE},
        {bounds.x + bounds.w / 2 + ARROW_SIZE / 2, y + ARROW_SIZE / 2},
        {bounds.x + bounds.w / 2 - ARROW_SIZE / 2, y + ARROW_SIZE / 2}};
    for (int i = 0; i < 2; i++) {
      SDL_RenderDrawLine(renderer, points[i].x, points[i].y, points[i + 1].x,
                         points[i + 1].y);
    }
    SDL_RenderDrawLine(renderer, points[2].x, points[2].y, points[0].x,
                       points[0].y);
  }
}

float RLAgent::getRemainingTime() {
  return std::fmax(m_episodeDuration - m_episodeTime, 0.0f);
}

float RLAgent::getLastActionConfidence() const {
  StateKey currentKey = m_currentState.toKey();
  std::string stateStr = currentKey.toString();

  if (m_qTable.find(stateStr) == m_qTable.end()) {
    return 0.0f;
  }

  // Find the maximum Q-value for the current state
  float maxQ = -std::numeric_limits<float>::max();
  float minQ = std::numeric_limits<float>::max();

  const auto &actionValues = m_qTable.at(stateStr);
  for (const auto &[action, value] : actionValues) {
    maxQ = std::max(maxQ, value);
    minQ = std::min(minQ, value);
  }

  // Get the Q-value of the last action
  float lastActionQ = actionValues.at(m_lastAction.type);

  // Normalize to [0,1] range
  float range = maxQ - minQ;
  if (range > 0) {
    return (lastActionQ - minQ) / range;
  }
  return 0.5f; // Default confidence if all values are equal
}

void RLAgent::applyAction(const Action &action) {
  const float MOVE_FORCE = 500.0f;
  bool isMoving = false;

  if (action.moveLeft || action.moveRight) {
    isMoving = true;
    m_character->isMoving = true;
    m_character->inputDirection = action.moveRight ? 1 : -1;
    m_character->mover.applyForce(
        Vector2f(MOVE_FORCE * (action.moveRight ? 1.0f : -1.0f), 0));

    // Only change to walk animation if not attacking
    if (m_character->animator->getCurrentFramePhase() == FramePhase::None) {
      m_character->animator->play("Walk");
    }
  } else {
    m_character->isMoving = false;
    m_character->inputDirection = 0;

    // Return to idle if not in another animation
    if (m_character->animator->getCurrentFramePhase() == FramePhase::None) {
      m_character->animator->play("Idle");
    }
  }

  if (action.jump && m_character->onGround) {
    m_character->jump();
  }

  if (action.attack) {
    FramePhase currentPhase = m_character->animator->getCurrentFramePhase();
    auto currentAnim = m_character->animator->getCurrentAnimationKey();

    // Combo system
    if (currentPhase == FramePhase::Recovery) {
      if (currentAnim == "Attack") {
        m_character->animator->play("Attack 2");
      } else if (currentAnim == "Attack 2") {
        m_character->animator->play("Attack 3");
      }
    } else if (currentPhase == FramePhase::None) {
      m_character->animator->play("Attack");
    }

    m_timeSinceLastAction = 0;
  }
}

void RLAgent::drawArrow(SDL_Renderer *renderer, int x1, int y1, int x2, int y2,
                        int arrowSize) {
  // Draw main line
  SDL_RenderDrawLine(renderer, x1, y1, x2, y2);

  // Calculate arrow head
  float angle = std::atan2(y2 - y1, x2 - x1);
  float a1 = angle + M_PI * 0.8f;
  float a2 = angle - M_PI * 0.8f;

  // Draw arrow head lines
  SDL_RenderDrawLine(renderer, x2, y2,
                     static_cast<int>(x2 - arrowSize * std::cos(a1)),
                     static_cast<int>(y2 - arrowSize * std::sin(a1)));
  SDL_RenderDrawLine(renderer, x2, y2,
                     static_cast<int>(x2 - arrowSize * std::cos(a2)),
                     static_cast<int>(y2 - arrowSize * std::sin(a2)));
}
