#include "Character.hpp"
#include <SDL.h>
#include <algorithm>
#include <iostream>

#include "Character.hpp"
#include <SDL.h>

Character::Character(Animator *anim)
    : animator(anim), health(100), maxHealth(100), onGround(false) {}

SDL_Rect Character::getCollisionRect() const {

  const auto &hitboxes = animator->getCurrentHitboxes();
  SDL_Rect collisionRect;
  bool foundCollisionBox = false;

  for (const auto &hb : hitboxes) {
    if (!hb.enabled || hb.dataType != 1)
      continue;

    SDL_Rect hbRect = {hb.x, hb.y, hb.w, hb.h};
    if (!foundCollisionBox) {
      collisionRect = hbRect;
      foundCollisionBox = true;
    } else {

      int x1 = std::min(collisionRect.x, hbRect.x);
      int y1 = std::min(collisionRect.y, hbRect.y);
      int x2 = std::max(collisionRect.x + collisionRect.w, hbRect.x + hbRect.w);
      int y2 = std::max(collisionRect.y + collisionRect.h, hbRect.y + hbRect.h);
      collisionRect.x = x1;
      collisionRect.y = y1;
      collisionRect.w = x2 - x1;
      collisionRect.h = y2 - y1;
    }
  }

  if (!foundCollisionBox) {

    collisionRect = animator->getCurrentFrameRect();
    std::cout << "[DEBUG] No collision hitbox found; using full frame rect.\n";
  } else {
    std::cout << "[DEBUG] Using collision hitbox: (" << collisionRect.x << ", "
              << collisionRect.y << ", " << collisionRect.w << ", "
              << collisionRect.h << ")\n";
  }

  collisionRect.x += static_cast<int>(mover.position.x);
  collisionRect.y += static_cast<int>(mover.position.y);

  return collisionRect;
}

void Character::update(float deltaTime) {

  if (!onGround) {
    mover.applyForce(Vector2f(0, GRAVITY));
  }

  mover.update(deltaTime);
  animator->update(deltaTime);

  if (mover.position.y >= GROUND_LEVEL) {
    mover.position.y = GROUND_LEVEL;
    mover.velocity.y = 0;
    onGround = true;
  } else {
    onGround = false;
  }
}

void Character::render(SDL_Renderer *renderer) {

  animator->render(renderer, static_cast<int>(mover.position.x),
                   static_cast<int>(mover.position.y));

  SDL_Rect collRect = getCollisionRect();
  SDL_Rect healthBar = {collRect.x, collRect.y - 10, collRect.w, 5};

  float ratio = static_cast<float>(health) / maxHealth;
  SDL_Rect healthFill = {healthBar.x, healthBar.y,
                         static_cast<int>(healthBar.w * ratio), healthBar.h};

  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  SDL_RenderFillRect(renderer, &healthBar);
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  SDL_RenderFillRect(renderer, &healthFill);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, &healthBar);
}

void Character::handleInput(const Uint8 *keystate) {
  const float moveForce = 500.0f;
  if (keystate[SDL_SCANCODE_LEFT]) {
    mover.applyForce(Vector2f(-moveForce, 0));
  }
  if (keystate[SDL_SCANCODE_RIGHT]) {
    mover.applyForce(Vector2f(moveForce, 0));
  }
  if (keystate[SDL_SCANCODE_SPACE] && onGround) {
    jump();
  }
}

void Character::jump() {

  mover.velocity.y = -500.0f;
  onGround = false;
}

void Character::applyDamage(int damage) {
  health -= damage;
  if (health < 0)
    health = 0;
}
