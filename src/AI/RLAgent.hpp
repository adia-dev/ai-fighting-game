#pragma once
#include "AI/NeuralNetwork.hpp"
#include "Core/Config.hpp"
#include "Game/Character.hpp"
#include "State.hpp"
#include <deque>
#include <memory>
#include <queue>
#include <random>
#include <vector>

struct PrioritizedExperience {
  Experience exp;
  float priority;
  bool operator<(const PrioritizedExperience &other) const {
    return priority < other.priority;
  }
};

class RLAgent {
public:
  RLAgent(Character *character, Config &config);
  void update(float deltaTime, const Character &opponent);
  void reset();
  void startNewEpoch();

  Action lastAction() const { return m_lastAction; }
  float totalReward() { return m_totalReward; }
  void reportWin(bool didWin);
  void incrementEpisodeCount();

  void setEpsilonParameters(float start, float min, float decay) {
    m_epsilon_start = start;
    m_epsilon_min = min;
    m_epsilon_decay = decay;
  }

  void setTrainingParameters(float gamma, float tau, float reward_scale) {
    m_gamma = gamma;
    m_tau = tau;
    m_reward_scale = reward_scale;
  }

  void setPERParameters(float alpha, float beta) {
    m_per_alpha = alpha;
    m_per_beta = beta;
  }

  float getEpsilon() const { return m_epsilon; }
  float getLearningRate() const { return m_learningRate; }
  float getDiscountFactor() const { return m_discountFactor; }
  void setParameters(float epsilon, float learningRate, float discountFactor) {
    m_epsilon = epsilon;
    m_learningRate = learningRate;
    m_discountFactor = discountFactor;
  }

  void updateTargetNetwork();
  const State &getCurrentState() const { return m_currentState; }

  int getEpisodeCount() const { return m_episodeCount; }

  int getTotalRounds() const { return m_totalRounds; }
  int getWins() const { return m_wins; }
  float getWinRate() const { return m_winRate; }

  void setBattleStyle(const BattleStyle &style) { m_battleStyle = style; }

  Stance getCurrentStance() const { return m_currentStance; }
  const std::deque<ActionType> &getActionHistory() const {
    return m_actionHistory;
  }
  const std::deque<ActionType> &getOpponentActionHistory() const {
    return m_opponentActionHistory;
  }
  std::vector<float> m_qValueHistory;
  std::unique_ptr<NeuralNetwork> onlineDQN;
  std::unique_ptr<NeuralNetwork> targetDQN;

private:
  std::vector<float> stateToVector(const State &state);
  State getCurrentState(const Character &opponent);
  Action selectAction(const State &state);
  float calculateReward(const State &state, const Action &action);
  void learn(const Experience &exp);
  void applyAction(const Action &action);
  void updateReplayBuffer(const Experience &exp);
  void sampleAndTrain();
  bool isPassiveNoOp(const Experience &exp);

  void updateStance(const State &state);
  Action predictOpponentAction(const State &state);
  void trackActionHistory(ActionType action, bool isOpponent);
  void decayEpsilon();
  void updateComboSystem(const Action &action);

  Character *m_character;
  State m_currentState;
  Action m_lastAction;
  float m_totalReward;
  float m_episodeTime;
  float m_episodeDuration;
  float m_timeSinceLastAction;
  float m_lastHealth;
  float m_currentActionDuration;
  float m_actionHoldDuration;
  int m_consecutiveWhiffs;
  float m_lastOpponentHealth;

  int state_dim;
  int num_actions;

  float m_epsilon;
  float m_learningRate;
  float m_discountFactor;
  int m_episodeCount;
  int updateCounter;
  int m_wins;
  int m_totalRounds;
  float m_winRate;

  std::priority_queue<PrioritizedExperience> replayBuffer;
  static const size_t MAX_REPLAY_BUFFER = 40000;
  static const size_t BATCH_SIZE = 32;

  std::random_device m_rd;
  std::mt19937 m_gen;
  std::uniform_real_distribution<float> m_dist;

  int m_moveHoldCounter;
  static const int MOVE_HOLD_TICKS = 10;

  BattleStyle m_battleStyle;

  Stance m_currentStance;
  std::deque<ActionType> m_actionHistory;
  std::deque<ActionType> m_opponentActionHistory;
  int m_comboCount;

  Config &m_config;

  Vector2f m_lastOpponentPosition;
  Vector2f m_opponentVelocity;

  std::vector<Experience> m_batchBuffer;

  float m_epsilon_min = 0.01f;
  float m_epsilon_decay = 0.995f;
  float m_epsilon_start = 1.0f;

  float m_gamma = 0.99f;
  float m_tau = 0.005f;
  float m_reward_scale = 1.0f;

  static constexpr size_t MIN_EXPERIENCES_BEFORE_TRAINING = 1000;
  static constexpr float PRIORITY_EPSILON = 1e-6f;
  float m_per_alpha = 0.6f;
  float m_per_beta = 0.4f;

  float calculatePriority(float td_error) const;
  float calculateImportanceWeight(float priority, float max_priority) const;
  void softUpdateTargetNetwork();
  std::vector<float> getActionMask(const State &state) const;
};
