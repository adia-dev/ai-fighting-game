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

  float opponentVelocityX;
  float opponentVelocityY;
  bool isCornered;
  std::array<ActionType, 10> lastActions;
  std::array<ActionType, 10> opponentLastActions;

  float predictedDistance;

  Stance currentStance;

  float myStamina;
  float myMaxStamina;
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
  float timePenalty;
  float hpRatioWeight;

  float distancePenalty;
};
