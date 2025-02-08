#pragma once
#include "Game/Character.hpp"

class FightSystem {
public:
  // Check for hits between attacker and defender.
  // If a hitbox from the attacker that is of type Hit and in Active phase
  // overlaps the defender's hurtbox (here we assume the defender’s collision
  // rectangle), apply damage and return true if a hit was registered.
  bool processHit(Character &attacker, Character &defender);
};
