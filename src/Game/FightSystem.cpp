#include "FightSystem.hpp"
#include "Data/Animation.hpp"
#include <SDL.h>
#include <iostream>

// Helper: check if two SDL_Rects intersect.
static bool rectIntersect(const SDL_Rect &a, const SDL_Rect &b) {
  return SDL_HasIntersection(&a, &b);
}

bool FightSystem::processHit(Character &attacker, Character &defender) {
  // For simplicity, we assume the attacker's current active hitboxes
  // are those in the current frame that are enabled, of type Hit,
  // and with phase == Active.
  const std::vector<Hitbox> &hitboxes = attacker.animator->getCurrentHitboxes();
  bool hitRegistered = false;
  SDL_Rect defenderHurtbox =
      defender.getCollisionRect(); // or use a dedicated hurtbox

  for (const auto &hb : hitboxes) {
    // Only consider hitboxes that are active (and of type Hit)
    if (!hb.enabled || hb.type != HitboxType::Hit)
      continue;

    // In a real system you might want each hitbox to store its phase,
    // but here we assume that the current frame is accessible
    // from the animator. (Alternatively, you could add a helper method.)
    // For this example, we assume that if the current frame phase is Active,
    // then the hitboxes are active.
    // (You might extend Animator to provide the current frame phase.)
    // We'll assume that the attacker’s current animation frame is active if:
    //   (attacker.walkAnimation.frames[attacker.animator->getCurrentFrameIndex()].phase
    //   == FramePhase::Active)
    // For simplicity, we assume the current hitbox should only register in
    // Active phase. (A more robust design would expose the current frame's
    // phase via Animator.) Here we simply assume that a hitbox in the "Hit"
    // type is active.

    // Compute the hitbox's absolute rectangle (position relative to the game
    // world).
    SDL_Rect hbRect = {static_cast<int>(attacker.mover.position.x + hb.x),
                       static_cast<int>(attacker.mover.position.y + hb.y), hb.w,
                       hb.h};

    if (rectIntersect(hbRect, defenderHurtbox)) {
      // Apply damage (for example, subtract 10 hit points).
      defender.applyDamage(10);
      std::cout << "[DEBUG] Hit registered! Damage applied.\n";
      hitRegistered = true;
      // In a more advanced system you might only register one hit per frame,
      // or track combos, etc.
      break;
    }
  }
  return hitRegistered;
}
