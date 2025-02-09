// src/AI/RLAgent.hpp
#pragma once
#include "Data/Vector2f.hpp"
#include "Game/Character.hpp"
#include <SDL.h>
#include <deque>
#include <random>
#include <sstream>
#include <vector>

// Discretized state for Q-table
struct StateKey {
  int distanceBucket;   // 0-4: very close, close, medium, far, very far
  int healthDiffBucket; // -2 to 2: much lower, lower, even, higher, much higher
  bool isAttacking;
  bool isOpponentAttacking;
  bool onGround;

  std::string toString() const {
    std::stringstream ss;
    ss << distanceBucket << "|" << healthDiffBucket << "|" << isAttacking << "|"
       << isOpponentAttacking << "|" << onGround;
    return ss.str();
  }
};

struct State {
  // Existing fields
  float distanceToOpponent;
  float relativePositionX;
  float relativePositionY;
  float myHealth;
  float opponentHealth;
  float timeSinceLastAction;
  float timeSinceLastDamage;
  bool amIOnGround;
  bool isOpponentOnGround;
  std::string myCurrentAnimation;
  std::string opponentCurrentAnimation;

  // New fields for better state representation
  bool isOpponentVulnerable;         // Track opponent's recovery state
  float myComboCounter;              // Track current combo count
  bool isOpponentBlocking;           // Track opponent's defensive state
  float distanceToWall;              // Consider arena boundaries
  Vector2f opponentMomentum;         // Track opponent's movement patterns
  std::vector<bool> availableSkills; // Track cooldown status of skills

  StateKey toKey() const {
    StateKey key;
    // Enhanced discretization
    key.distanceBucket =
        std::min(4, static_cast<int>(distanceToOpponent / 100.0f));

    // More nuanced health difference buckets
    float healthDiff = myHealth - opponentHealth;
    key.healthDiffBucket =
        std::clamp(static_cast<int>(healthDiff * 2.5f), -2, 2);

    key.isAttacking = myCurrentAnimation.find("Attack") != std::string::npos;
    key.isOpponentAttacking =
        opponentCurrentAnimation.find("Attack") != std::string::npos;
    key.onGround = amIOnGround;

    return key;
  }
};

enum class ActionType {
  None,
  MoveLeft,
  MoveRight,
  Jump,
  Attack,
  // New combined actions
  JumpAttack,
  MoveLeftAttack,
  MoveRightAttack
};

inline const char *actionTypeToString(ActionType actionType) {
  switch (actionType) {
  case ActionType::None:
    return "None";
  case ActionType::MoveLeft:
    return "MoveLeft";
  case ActionType::MoveRight:
    return "MoveRight";
  case ActionType::Jump:
    return "Jump";
  case ActionType::Attack:
    return "Attack";
  case ActionType::JumpAttack:
    return "JumpAttack";
  case ActionType::MoveLeftAttack:
    return "MoveLeftAttack";
  case ActionType::MoveRightAttack:
    return "MoveRightAttack";
  }
}

struct Action {
  ActionType type;
  bool moveLeft;
  bool moveRight;
  bool jump;
  bool attack;

  static Action fromType(ActionType type) {
    Action a{};
    a.type = type;
    switch (type) {
    case ActionType::MoveLeft:
      a.moveLeft = true;
      break;
    case ActionType::MoveRight:
      a.moveRight = true;
      break;
    case ActionType::Jump:
      a.jump = true;
      break;
    case ActionType::Attack:
      a.attack = true;
      break;
    case ActionType::JumpAttack:
      a.jump = true;
      a.attack = true;
      break;
    case ActionType::MoveLeftAttack:
      a.moveLeft = true;
      a.attack = true;
      break;
    case ActionType::MoveRightAttack:
      a.moveRight = true;
      a.attack = true;
      break;
    default:
      break;
    }
    return a;
  }
};

struct Experience {
  State state;
  Action action;
  float reward;
  State nextState;
  bool done;
};

struct EpisodeStats {
  int totalEpisodes;
  int winsCount;
  float winRate;
  float avgReward;
  float bestReward;
  std::vector<float> rewardHistory;
};

class RLAgent {
public:
  RLAgent(Character *character);

  void update(float deltaTime, const Character &opponent);
  void render(SDL_Renderer *renderer);
  void reset();
  void startNewEpoch();

  void setEpisodeDuration(float duration) { m_episodeDuration = duration; }

private:
  // Core RL components
  State getCurrentState(const Character &opponent);
  Action selectAction(const State &state);
  float calculateReward(const State &state, const Action &action);
  void learn(const Experience &exp);
  void updateRadar();
  void updateStats();
  void initQTable();
  float getRemainingTime();

  // Visualization
  void renderRadar(SDL_Renderer *renderer);
  void renderDebugInfo(SDL_Renderer *renderer);
  void drawArrow(SDL_Renderer *renderer, int x1, int y1, int x2, int y2,
                 int arrowSize);
  float calculateActionDuration(const Action &action);
  void renderStateValues(SDL_Renderer *renderer, const SDL_Rect &bounds);
  void applyAction(const Action &action);
  float getLastActionConfidence() const;
  void renderActionIntentions(SDL_Renderer *renderer, const SDL_Rect &bounds);

  void renderStateDisplay(SDL_Renderer *renderer);
  void drawHorizontalGauge(SDL_Renderer *renderer, int x, int y, int width,
                           int height, float value, SDL_Color color);
  void updateStateDisplay() {
    // Update any state display related data here
    // This could include updating graphs, collecting statistics, etc.
  }
  void logDecision(const State &state, const Action &action, float reward);

  Character *m_character;
  State m_currentState;
  Action m_lastAction;
  float m_totalReward;
  float m_episodeTime;
  float m_episodeDuration;
  float m_timeSinceLastAction;
  float m_timeSinceLastDamage;
  float m_lastHealth;
  float m_currentActionDuration = 0.0f;
  float m_actionHoldDuration = 0.1f;
  int m_consecutiveWhiffs = 0;

  std::map<std::string, std::map<ActionType, float>> m_qTable;

  // Experience replay buffer
  std::deque<Experience> m_replayBuffer;
  static const size_t MAX_REPLAY_BUFFER = 10000;

  // Training parameters
  float m_epsilon;
  float m_learningRate;
  float m_discountFactor;
  int m_episodeCount;

  // Radar visualization
  struct RadarBlip {
    Vector2f position;
    float intensity;
  };
  std::vector<RadarBlip> m_radarBlips;
  EpisodeStats m_stats;

  // Random number generation
  std::random_device m_rd;
  std::mt19937 m_gen;
  std::uniform_real_distribution<float> m_dist;
};
