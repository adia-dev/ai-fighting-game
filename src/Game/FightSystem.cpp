#include "FightSystem.hpp"
#include "Data/Animation.hpp"
#include "Game/CollisionSystem.hpp"
#include <SDL.h>
#include <string>

bool FightSystem::processHit(Character &attacker, Character &defender) {
  // Get the current animation being played
  std::string currentAnimationKey = attacker.animator->getCurrentAnimationKey();

  // Create a unique key for this attacker-defender pair
  auto hitKey = std::make_pair(&attacker, &defender);
  auto &hitReg = m_hitRegistrations[hitKey];

  // If we already registered a hit for this attack animation, ignore further
  // collisions
  if (hitReg.currentAttackAnimation == currentAnimationKey &&
      hitReg.hitCooldown > 0) {
    return false;
  }

  const std::vector<Hitbox> &hitboxes = attacker.animator->getCurrentHitboxes();
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
      defender.applyDamage(15, true);
      defender.block();
      defender.lastBlockEffective = true;
      attacker.lastAttackLanded = false;

      // Register the hit and start cooldown
      hitReg.hitCooldown = HIT_COOLDOWN_DURATION;
      hitReg.currentAttackAnimation = currentAnimationKey;
      return true;
    }

    // Then check for hit
    if (CollisionSystem::checkCollision(hbRect, defenderHurtbox)) {
      int randomHitAnimation = rand() % 3 + 1;
      if (randomHitAnimation == 1) {
        defender.animator->play("Hit");
      } else {
        defender.animator->play("Hit " + std::to_string(randomHitAnimation));
      }
      attacker.lastAttackLanded = true;
      defender.lastBlockEffective = false;

      float baseImpulse = 500.0f;
      defender.applyDamage(35 * attacker.comboCount * 1.25f);
      float knockbackForce = baseImpulse * (1.0f + attacker.comboCount * 0.1f);
      CollisionSystem::applyCollisionImpulse(attacker, defender,
                                             knockbackForce);

      // Register the hit and start cooldown
      hitReg.hitCooldown = HIT_COOLDOWN_DURATION;
      hitReg.currentAttackAnimation = currentAnimationKey;
      return true;
    }
  }
  return false;
}

void FightSystem::update(float deltaTime) {
  // Update all hit registration cooldowns
  for (auto &pair : m_hitRegistrations) {
    auto &hitReg = pair.second;
    if (hitReg.hitCooldown > 0) {
      hitReg.hitCooldown -= deltaTime;
      if (hitReg.hitCooldown <= 0) {
        // Reset the attack animation tracking when cooldown expires
        hitReg.currentAttackAnimation.clear();
      }
    }
  }
}
