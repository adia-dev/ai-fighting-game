#include "FightSystem.hpp"
#include "Data/Animation.hpp"
#include "Game/CollisionSystem.hpp"
#include <SDL.h>
#include <string>

bool FightSystem::processHit(Character &attacker, Character &defender) {
  const std::vector<Hitbox> &hitboxes = attacker.animator->getCurrentHitboxes();
  bool hitRegistered = false;
  SDL_Rect defenderHurtbox = defender.getHitboxRect();
  SDL_Rect defenderBlockBox = defender.getHitboxRect(HitboxType::Block);
  SDL_Rect attackerFrameRect = attacker.animator->getCurrentFrameRect();
  bool isAttackerFlipped = attacker.animator->getFlip();

  for (const auto &hb : hitboxes) {
    if (!hb.enabled || hb.type != HitboxType::Hit)
      continue;

    // Calculate hitbox position taking flipping into account
    SDL_Rect hbRect;
    if (isAttackerFlipped) {
      hbRect.x = static_cast<int>(attacker.mover.position.x +
                                  (attackerFrameRect.w - (hb.x + hb.w)));
    } else {
      hbRect.x = static_cast<int>(attacker.mover.position.x + hb.x);
    }
    hbRect.y = static_cast<int>(attacker.mover.position.y + hb.y);
    hbRect.w = hb.w;
    hbRect.h = hb.h;

    // Check for block first
    if (CollisionSystem::checkCollision(hbRect, defenderBlockBox)) {
      defender.applyDamage(5, true);
      defender.block();
      defender.lastBlockEffective = true;
      attacker.lastAttackLanded = false;
      hitRegistered = true;
      break;
    }

    // Then check for hit
    if (CollisionSystem::checkCollision(hbRect, defenderHurtbox)) {
      int randomHitAnimation = rand() % 3 + 1;
      defender.animator->play("Hit " + std::to_string(randomHitAnimation));
      attacker.lastAttackLanded = true;
      defender.lastBlockEffective = false;

      // TODO: Make it dynamic based on the attack (should be on the state of
      // the hitbox)
      float baseImpulse = 200.0f;
      defender.applyDamage(80 * attacker.comboCount * 1.25f);
      float knockbackForce = baseImpulse * (1.0f + attacker.comboCount * 0.1f);
      CollisionSystem::applyCollisionImpulse(attacker, defender,
                                             knockbackForce);
      hitRegistered = true;
      break;
    }
  }
  return hitRegistered;
}
