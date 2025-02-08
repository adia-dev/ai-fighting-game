
#include "Character.hpp"
#include <SDL.h>

Character::Character(Animator *anim)
    : animator(anim), health(100), maxHealth(100), onGround(false) {}

SDL_Rect Character::getCollisionRect() const {
  SDL_Rect frameRect = animator->getCurrentFrameRect();
  frameRect.x += static_cast<int>(mover.position.x);
  frameRect.y += static_cast<int>(mover.position.y);
  return frameRect;
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
