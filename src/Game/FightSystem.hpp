#pragma once
#include "Game/Character.hpp"

class FightSystem {
public:
  bool processHit(Character &attacker, Character &defender);
  void update(float deltaTime);

private:
  // Track when a hit was last registered for each attacker-defender pair
  struct HitRegistration {
    float hitCooldown = 0.0f; // Time until next hit can be registered
    std::string
        currentAttackAnimation; // Track which attack animation caused the hit
  };

  // Use a map to track hit registration between characters
  // The key is a pair of pointers to identify unique attacker-defender
  // combinations
  std::map<std::pair<Character *, Character *>, HitRegistration>
      m_hitRegistrations;

  static constexpr float HIT_COOLDOWN_DURATION = 0.5f;
};
