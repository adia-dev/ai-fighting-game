// src/Game/FightSystem.cpp
#include "FightSystem.hpp"
#include "Core/Logger.hpp"
#include "Data/Animation.hpp"
#include "Game/CollisionSystem.hpp"
#include <SDL.h>

bool FightSystem::processHit(Character &attacker, Character &defender) {
  const std::vector<Hitbox> &hitboxes = attacker.animator->getCurrentHitboxes();
  bool hitRegistered = false;
  SDL_Rect defenderHurtbox = defender.getHitboxRect();
  SDL_Rect defenderBlockBox = defender.getHitboxRect(HitboxType::Block);

  for (const auto &hb : hitboxes) {
    if (!hb.enabled || hb.type != HitboxType::Hit)
      continue;
    SDL_Rect hbRect = {static_cast<int>(attacker.mover.position.x + hb.x),
                       static_cast<int>(attacker.mover.position.y + hb.y), hb.w,
                       hb.h};

    if (CollisionSystem::checkCollision(hbRect, defenderBlockBox)) {
      defender.applyDamage(2, true);
      Logger::debug("Block registered! Small Damage applied.");
      defender.animator->play("Block");
      hitRegistered = true;
      break;
    }

    if (CollisionSystem::checkCollision(hbRect, defenderHurtbox)) {
      defender.applyDamage(10);
      Logger::debug("Hit registered! Damage applied.");
      defender.animator->play("Hit");
      hitRegistered = true;
      break;
    }
  }
  return hitRegistered;
}
