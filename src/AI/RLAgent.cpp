#include "RLAgent.hpp"
#include "Core/Logger.hpp"
#include <algorithm>

// Default values.
static constexpr float DEFAULT_EPISODE_DURATION = 60.0f;
static constexpr int TARGET_UPDATE_FREQUENCY = 1000; // steps

RLAgent::RLAgent(Character *character)
    : m_character(character), m_totalReward(0), m_episodeTime(0),
      m_episodeDuration(DEFAULT_EPISODE_DURATION), m_timeSinceLastAction(0),
      m_lastHealth(100), m_currentActionDuration(0), m_actionHoldDuration(0.2f),
      m_consecutiveWhiffs(0), m_epsilon(0.3f), m_learningRate(0.001f),
      m_discountFactor(0.95f), m_episodeCount(0), updateCounter(0),
      m_gen(m_rd()), m_dist(0.0f, 1.0f) {
  // Our state vector now has 6 base features + 4 radar features = 10.
  state_dim = 10;
  num_actions = 9; // 8 original actions + Block action.
  onlineDQN = std::make_unique<NeuralNetwork>(state_dim, 64, num_actions);
  targetDQN = std::make_unique<NeuralNetwork>(state_dim, 64, num_actions);
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
  // Append radar features (assumed to be 4 values).
  for (int i = 0; i < 4; i++)
    v.push_back(state.radar[i]);
  return v;
}

// In src/AI/RLAgent.cpp:
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

  // Compute four simple radar features (for instance, by quadrants).
  state.radar[0] =
      (toOpponent.x >= 0 && toOpponent.y >= 0) ? toOpponent.length() : 0;
  state.radar[1] =
      (toOpponent.x < 0 && toOpponent.y >= 0) ? toOpponent.length() : 0;
  state.radar[2] =
      (toOpponent.x < 0 && toOpponent.y < 0) ? toOpponent.length() : 0;
  state.radar[3] =
      (toOpponent.x >= 0 && toOpponent.y < 0) ? toOpponent.length() : 0;

  // Initialize m_lastOpponentHealth on the first call.
  if (m_lastOpponentHealth == 0)
    m_lastOpponentHealth = state.opponentHealth;

  return state;
}

Action RLAgent::selectAction(const State &state) {
  auto state_vec = stateToVector(state);
  auto q_values = onlineDQN->forward(state_vec);
  if (m_dist(m_gen) < m_epsilon) {
    int random_action = static_cast<int>(m_dist(m_gen) * num_actions);
    return Action::fromType(static_cast<ActionType>(random_action));
  }
  int bestAction = std::distance(
      q_values.begin(), std::max_element(q_values.begin(), q_values.end()));
  return Action::fromType(static_cast<ActionType>(bestAction));
}

float RLAgent::calculateReward(const State &state, const Action &action) {
  float reward = 0.0f;

  // (1) Health margin: reward if you have higher health than your opponent.
  float healthMargin = state.myHealth - state.opponentHealth;
  reward += healthMargin * 10.0f;

  // (2) If an attack was performed, reward if it landed, or penalize otherwise.
  if (action.attack) {
    // If our character’s last attack landed (flag set by FightSystem)
    if (m_character->lastAttackLanded) {
      // Bonus reward proportional to damage (here, assume 10 damage per hit)
      reward += 20.0f;
    } else {
      // Penalty for wasted attack
      reward -= 5.0f;
    }
  }

  // (3) If block was used, reward it if effective (i.e. blocking an incoming
  // attack)
  if (action.block) {
    if (m_character->lastBlockEffective) {
      reward += 10.0f;
    } else {
      reward -= 3.0f;
    }
  }

  // (4) Distance shaping: reward staying close to the optimal range.
  float optimalDistance = 150.0f;
  reward -= std::abs(state.distanceToOpponent - optimalDistance) * 0.05f;

  // (5) Time penalty: penalize for inactivity to encourage fast, decisive
  // actions.
  reward -= state.timeSinceLastAction * 0.1f;

  // (6) (Optional) Penalize consecutive missed actions (e.g. excessive whiffs).
  if (action.attack && !m_character->lastAttackLanded)
    reward -= m_consecutiveWhiffs * 2.0f;

  return reward;
}

void RLAgent::updateReplayBuffer(const Experience &exp) {
  if (replayBuffer.size() >= MAX_REPLAY_BUFFER)
    replayBuffer.pop_front();
  replayBuffer.push_back(exp);
}

void RLAgent::sampleAndTrain() {
  if (replayBuffer.size() < BATCH_SIZE)
    return;
  std::vector<Experience> batch;
  std::uniform_int_distribution<size_t> indexDist(0, replayBuffer.size() - 1);
  for (size_t i = 0; i < BATCH_SIZE; ++i)
    batch.push_back(replayBuffer[indexDist(m_gen)]);

  for (auto &exp : batch) {
    auto s = stateToVector(exp.state);
    auto s_next = stateToVector(exp.nextState);
    auto current_q = onlineDQN->forward(s);
    auto next_q = targetDQN->forward(s_next);
    float max_next_q = *std::max_element(next_q.begin(), next_q.end());
    int action_index = static_cast<int>(exp.action.type);
    current_q[action_index] = exp.reward + m_discountFactor * max_next_q;
    onlineDQN->train(s, current_q, m_learningRate);
  }
  updateCounter += BATCH_SIZE;
  if (updateCounter >= TARGET_UPDATE_FREQUENCY) {
    // In a proper implementation, copy parameters.
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
    m_character->block(); // You must implement Character::block()
}

void RLAgent::update(float deltaTime, const Character &opponent) {
  m_episodeTime += deltaTime;
  m_timeSinceLastAction += deltaTime;
  m_currentActionDuration += deltaTime;

  State newState = getCurrentState(opponent);
  // Only select a new action after the hold duration expires.
  if (m_currentActionDuration >= m_actionHoldDuration) {
    Action newAction = selectAction(newState);

    // If the new action is of the same movement type as the last one,
    // extend the hold duration (smoother movement).
    if ((newAction.moveLeft && m_lastAction.moveLeft) ||
        (newAction.moveRight && m_lastAction.moveRight)) {
      m_actionHoldDuration = 0.5f; // longer hold for movement
    } else {
      m_actionHoldDuration = 0.3f;
    }

    // Update reward signals based on health differences.
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
    Logger::debug("Selected action: %s", actionTypeToString(m_lastAction.type));
  }
  applyAction(m_lastAction);
}

void RLAgent::reset() {
  m_totalReward = 0;
  m_episodeTime = 0;
  m_currentState = getCurrentState(*m_character);
  m_lastAction = Action::fromType(ActionType::None);
  replayBuffer.clear();
}

void RLAgent::startNewEpoch() {
  m_character->mover.position = {100, 100};
  m_character->health = m_character->maxHealth;
  reset();
  Logger::info("Starting new epoch, reward reset.");
}
