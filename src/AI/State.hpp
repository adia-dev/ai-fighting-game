#pragma once

#include <array>
enum class ActionType {
  Noop,
  MoveLeft,
  MoveRight,
  Jump,
  Attack,
  Block,
  JumpAttack,
  MoveLeftAttack,
  MoveRightAttack
};

inline const char *actionTypeToString(ActionType type) {
  switch (type) {
  case ActionType::Noop:
    return "No-op";
  case ActionType::MoveLeft:
    return "MoveLeft";
  case ActionType::MoveRight:
    return "MoveRight";
  case ActionType::Jump:
    return "Jump";
  case ActionType::Attack:
    return "Attack";
  case ActionType::Block:
    return "Block";
  case ActionType::JumpAttack:
    return "JumpAttack";
  case ActionType::MoveLeftAttack:
    return "MoveLeftAttack";
  case ActionType::MoveRightAttack:
    return "MoveRightAttack";
  default:
    return "Unknown";
  }
}

enum class Stance { Neutral, Aggressive, Defensive };

struct State {
  float distanceToOpponent;
  float relativePositionX;
  float relativePositionY;
  float myHealth;
  float opponentHealth;
  float timeSinceLastAction;
  float radar[4];

  float opponentVelocityX; // Opponent’s horizontal velocity
  float opponentVelocityY; // Opponent’s vertical velocity
  bool isCornered;         // True if the agent is near the stage edge
  std::array<ActionType, 3> lastActions; // Last 3 actions taken by the AI
  std::array<ActionType, 3> opponentLastActions; // Last 3 opponent actions

  // A prediction of the opponent distance in near-future
  float predictedDistance;

  // Current stance (for tactical adjustments)
  Stance currentStance;

  // New: normalized stamina (0.0 - 1.0) and max stamina (could be 1.0 if
  // normalized)
  float myStamina; // e.g., current stamina normalized
  float
      myMaxStamina; // e.g., maximum stamina (could be always 1.0 if normalized)
};

struct Action {
  ActionType type;
  bool moveLeft;
  bool moveRight;
  bool jump;
  bool attack;
  bool block;

  static Action fromType(ActionType type) {
    Action a{};
    a.type = type;
    a.moveLeft =
        (type == ActionType::MoveLeft || type == ActionType::MoveLeftAttack);
    a.moveRight =
        (type == ActionType::MoveRight || type == ActionType::MoveRightAttack);
    a.jump = (type == ActionType::Jump || type == ActionType::JumpAttack);
    a.attack = (type == ActionType::Attack || type == ActionType::JumpAttack ||
                type == ActionType::MoveLeftAttack ||
                type == ActionType::MoveRightAttack);
    a.block = (type == ActionType::Block);
    return a;
  }
};

struct Experience {
  State state;
  Action action;
  float reward;
  State nextState;
};

struct BattleStyle {
  float timePenalty;     // Penalty per time step (e.g., 0.004 for balanced)
  float hpRatioWeight;   // Multiplier for health margin reward (e.g., 1.0 for
                         // balanced)
  float distancePenalty; // Penalty per unit deviation from optimal distance
                         // (e.g., 0.0002 for balanced)
};
