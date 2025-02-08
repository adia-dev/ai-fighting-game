// src/Game/FightSystem.cpp
#include "FightSystem.hpp"
#include "Core/Logger.hpp"
#include "Data/Animation.hpp"
#include <SDL.h>

static bool rectIntersect(const SDL_Rect &a, const SDL_Rect &b) {
  return SDL_HasIntersection(&a, &b);
}

bool FightSystem::processHit(Character &attacker, Character &defender) {
  const std::vector<Hitbox> &hitboxes = attacker.animator->getCurrentHitboxes();
  bool hitRegistered = false;
  SDL_Rect defenderHurtbox = defender.getCollisionRect();

  for (const auto &hb : hitboxes) {
    if (!hb.enabled || hb.type != HitboxType::Hit)
      continue;
    SDL_Rect hbRect = {static_cast<int>(attacker.mover.position.x + hb.x),
                       static_cast<int>(attacker.mover.position.y + hb.y), hb.w,
                       hb.h};

    if (rectIntersect(hbRect, defenderHurtbox)) {
      defender.applyDamage(10);
      Logger::debug("Hit registered! Damage applied.");
      hitRegistered = true;
      break;
    }
  }
  return hitRegistered;
}
