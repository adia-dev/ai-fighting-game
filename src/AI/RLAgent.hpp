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

// Struct for prioritized experience replay.
struct PrioritizedExperience {
  Experience exp;
  float priority;
  bool operator<(const PrioritizedExperience &other) const {
    // Higher priority comes first.
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

  float getEpsilon() const { return m_epsilon; }
  float getLearningRate() const { return m_learningRate; }
  float getDiscountFactor() const { return m_discountFactor; }
  void setParameters(float epsilon, float learningRate, float discountFactor) {
    m_epsilon = epsilon;
    m_learningRate = learningRate;
    m_discountFactor = discountFactor;
  }

  // State access
  const State &getCurrentState() const { return m_currentState; }

  // Statistics
  int getEpisodeCount() const { return m_episodeCount; }
  float getWinRate() const {
    return m_wins / (float)std::max(1, m_episodeCount);
  }

  // Set the battle style for reward shaping.
  void setBattleStyle(const BattleStyle &style) { m_battleStyle = style; }

  // Accessor methods for debugging.
  Stance getCurrentStance() const { return m_currentStance; }
  const std::deque<ActionType> &getActionHistory() const {
    return m_actionHistory;
  }
  const std::deque<ActionType> &getOpponentActionHistory() const {
    return m_opponentActionHistory;
  }
  std::vector<float> m_qValueHistory; // For plotting Q-values in debug
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

  // New helper methods.
  void updateStance(const State &state);
  Action predictOpponentAction(const State &state);
  void trackActionHistory(ActionType action, bool isOpponent);
  void decayEpsilon();
  void updateComboSystem(const Action &action);

  // Core members.
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

  // Prioritized replay buffer.
  std::priority_queue<PrioritizedExperience> replayBuffer;
  static const size_t MAX_REPLAY_BUFFER = 40000;
  static const size_t BATCH_SIZE = 32;

  std::random_device m_rd;
  std::mt19937 m_gen;
  std::uniform_real_distribution<float> m_dist;

  // For maintaining move decisions.
  int m_moveHoldCounter;
  static const int MOVE_HOLD_TICKS = 10;

  // For reward shaping.
  BattleStyle m_battleStyle;

  // Tactical stance and action histories.
  Stance m_currentStance;
  std::deque<ActionType> m_actionHistory;
  std::deque<ActionType> m_opponentActionHistory;
  int m_comboCount;

  Config &m_config;

  // For opponent prediction.
  Vector2f m_lastOpponentPosition;
  Vector2f m_opponentVelocity;
};
