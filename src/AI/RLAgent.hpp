// RLAgent.hpp
#pragma once
#include "AI/NeuralNetwork.hpp"
#include "Game/Character.hpp"
#include "State.hpp"
#include <deque>
#include <memory>
#include <random>
#include <vector>

class RLAgent {
public:
  RLAgent(Character *character);
  void update(float deltaTime, const Character &opponent);
  void reset();
  void startNewEpoch();

  Action lastAction() { return m_lastAction; }
  float totalReward() { return m_totalReward; }

private:
  std::vector<float> stateToVector(const State &state);
  State getCurrentState(const Character &opponent);
  Action selectAction(const State &state);
  float calculateReward(const State &state, const Action &action);
  void learn(const Experience &exp);
  void applyAction(const Action &action);
  void updateReplayBuffer(const Experience &exp);
  void sampleAndTrain();

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

  // Neural networks for Double DQN.
  std::unique_ptr<NeuralNetwork> onlineDQN;
  std::unique_ptr<NeuralNetwork> targetDQN;
  int state_dim;
  int num_actions;

  // Training hyperparameters.
  float m_epsilon;
  float m_learningRate;
  float m_discountFactor;
  int m_episodeCount;
  int updateCounter;

  // Experience replay (if desired)
  std::deque<Experience> replayBuffer;
  static const size_t MAX_REPLAY_BUFFER = 40000;
  static const size_t BATCH_SIZE = 32;

  std::random_device m_rd;
  std::mt19937 m_gen;
  std::uniform_real_distribution<float> m_dist;
};
