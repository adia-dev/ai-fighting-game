#pragma once

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

struct State {
  // Original features.
  float distanceToOpponent;
  float relativePositionX;
  float relativePositionY;
  float myHealth;
  float opponentHealth;
  float timeSinceLastAction;
  // New radar features (for example, four values representing quadrants).
  float radar[4];
  // (Optionally, you could add history features here, for example a fixed
  // window of previous action indices.)
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
