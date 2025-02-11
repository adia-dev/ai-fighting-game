#include "Character.hpp"
#include "Core/DebugEvents.hpp"
#include "Core/DebugGlobals.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Data/Animation.hpp"
#include "Data/Vector2f.hpp"
#include <SDL.h>
#include <algorithm>

Character::Character(Animator *anim, Config &config)
    : animator(anim), health(100), maxHealth(100), onGround(false),
      isMoving(false), groundFrames(0), inputDirection(0), stamina(500.0f),
      maxStamina(500.0f), m_config(config) {}

SDL_Rect Character::getHitboxRect(HitboxType type) const {
  const auto &hitboxes = animator->getCurrentHitboxes();
  SDL_Rect collisionRect;
  bool foundCollisionBox = false;

  SDL_Rect frameRect = animator->getCurrentFrameRect();

  for (const auto &hb : hitboxes) {
    if (!hb.enabled || hb.type != type)
      continue;

    SDL_Rect hbRect;
    if (animator->getFlip()) {

      hbRect.x = frameRect.w - (hb.x + hb.w);
    } else {
      hbRect.x = hb.x;
    }
    hbRect.y = hb.y;
    hbRect.w = hb.w;
    hbRect.h = hb.h;

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

    collisionRect = {frameRect.w / 4, frameRect.h / 4, frameRect.w / 2,
                     frameRect.h / 2};
  }

  collisionRect.x += static_cast<int>(mover.position.x);
  collisionRect.y += static_cast<int>(mover.position.y);
  return collisionRect;
}

void Character::handleInput() {
  FramePhase phase = animator->getCurrentFramePhase();
  if (phase == FramePhase::Startup || phase == FramePhase::Active) {
    return;
  }

  const float moveForce = m_config.moveForce;
  isMoving = false;
  inputDirection = 0;

  if (Input::isKeyDown(SDL_SCANCODE_LEFT)) {
    mover.applyForce(Vector2f(-moveForce, 0));
    isMoving = true;
    inputDirection = -1;
  }
  if (Input::isKeyDown(SDL_SCANCODE_RIGHT)) {
    mover.applyForce(Vector2f(moveForce, 0));
    isMoving = true;
    inputDirection = 1;
  }
  if (Input::isKeyDown(SDL_SCANCODE_A)) {
    attack();
  }
  if (Input::isKeyDown(SDL_SCANCODE_D)) {
    mover.applyForce(Vector2f(moveForce * 4, 0));
    mover.velocity.y = -300.0f;
    dash();
  }
  if (Input::isKeyDown(SDL_SCANCODE_B)) {
    block();
  }
  if (Input::isKeyDown(SDL_SCANCODE_SPACE) && onGround &&
      groundFrames >= m_config.stableGroundFrames) {
    jump();
  }
}

void Character::attack() {

  const float attackCost = 20.0f;
  if (stamina < attackCost) {
    Logger::debug("Not enough stamina for attack!");

    return;
  }

  stamina -= attackCost;

  FramePhase phase = animator->getCurrentFramePhase();
  auto currentAnimationKey = animator->getCurrentAnimationKey();

  if (phase == FramePhase::Recovery) {
    if (currentAnimationKey == "Attack") {
      animator->play("Attack 2");
      Logger::debug("Combo x2, Launching second attack!");
      return;
    } else if (currentAnimationKey == "Attack 2") {
      animator->play("Attack 3");
      Logger::debug("Combo x3, Launching third attack!");
      return;
    }
  }

  if (phase != FramePhase::Active && phase != FramePhase::Startup) {
    animator->play("Attack");
    Logger::debug("Attack initiated.");
  }
}

void Character::dash() {
  const float dashCost = 15.0f;
  if (stamina < dashCost) {
    Logger::debug("Not enough stamina for dash!");
    return;
  }
  stamina -= dashCost;

  FramePhase phase = animator->getCurrentFramePhase();
  if (phase == FramePhase::Active)
    return;
  animator->play("Dash");
}

void Character::block() {
  const float blockCost = 10.0f;
  if (stamina < blockCost) {
    Logger::debug("Not enough stamina for block!");
    return;
  }
  stamina -= blockCost;

  FramePhase phase = animator->getCurrentFramePhase();
  if (phase == FramePhase::Active)
    return;
  animator->play("Block");
}

void Character::jump() {
  groundFrames = 0;
  mover.velocity.y = m_config.jumpVelocity;
  onGround = false;
  Logger::debug("Jump initiated.");
}

void Character::move(const Vector2f &force) { mover.applyForce(force); }

void Character::applyDamage(int damage, bool survive) {
  bool isBlocking = animator->getCurrentAnimationKey() == "Block";
  health -= damage * (isBlocking ? 0.1f : 1.f);
  if (health < 0)
    health = survive ? 1 : 0;
  Logger::debug("Damage applied: " + std::to_string(damage) +
                ". Health now: " + std::to_string(health));

  if (g_showFloatingDamage) {
    addDamageEvent(mover.position, damage);
  }

  if (comboCount >= 2 && animator->getCurrentAnimationKey() != "Knocked") {
    animator->play("Knocked");
  }

  if (health <= 0 && animator->getCurrentAnimationKey() != "Die") {
    animator->play("Die");
  }
}

void Character::update(float deltaTime) {
  std::string currentAnim = animator->getCurrentAnimationKey();
  if (currentAnim != "Idle" && currentAnim != "Walk") {
    m_currentAnimationTimer += deltaTime;
    if (m_currentAnimationTimer > MAX_ANIMATION_DURATION) {
      Logger::debug("Animation '" + currentAnim +
                    "' stuck for too long, reverting to Idle");
      animator->play("Idle");
      m_currentAnimationTimer = 0.0f;
    }
  } else {
    m_currentAnimationTimer = 0.0f;
  }

  SDL_Rect collRect = getHitboxRect();
  if (collRect.h == 0) {
    collRect = animator->getCurrentFrameRect();
  }

  if (!onGround) {
    mover.applyForce(Vector2f(0, m_config.gravity));
  }

  collRect = getHitboxRect();
  if (collRect.h == 0) {
    collRect = animator->getCurrentFrameRect();
  }

  float charBottom = mover.position.y + collRect.h;
  float groundY = m_config.groundLevel;

  if (charBottom >= groundY) {
    mover.position.y = groundY - collRect.h;
    mover.velocity.y = 0;
    onGround = true;
    groundFrames++;
  } else if (charBottom < groundY - m_config.groundThreshold) {
    onGround = false;
    groundFrames = 0;
  }

  const float staminaRecoveryRate = 50.0f;
  stamina = std::min(maxStamina, stamina + staminaRecoveryRate * deltaTime);

  mover.update(deltaTime);
  animator->update(deltaTime);

  FramePhase currentPhase = animator->getCurrentFramePhase();
  if (currentPhase == FramePhase::Active ||
      currentPhase == FramePhase::Startup) {
    return;
  }

  if (animator->getCurrentAnimationKey() == "Attack" &&
      animator->isAnimationFinished()) {
    animator->play("Idle");
  }

  updateJumpAnimation();

  if (onGround && animator->getCurrentAnimationKey() == "Landing") {
    if (animator->isAnimationFinished()) {
      animator->play("Idle");
    }
  }

  if (currentPhase == FramePhase::Active ||
      currentPhase == FramePhase::Startup) {

    return;
  }

  if (currentPhase == FramePhase::Recovery &&
      !animator->isAnimationFinished()) {
    return;
  }

  bool shouldBeIdle = !isMoving || (std::abs(mover.velocity.x) < 1.0f &&
                                    std::abs(mover.velocity.y) < 1.0f);
  if (shouldBeIdle) {
    if (animator->getCurrentAnimationKey() != "Idle")
      animator->play("Idle");
  } else if (isMoving && animator->getCurrentAnimationKey() != "Walk") {
    animator->play("Walk");
  }
}

void Character::render(SDL_Renderer *renderer, float cameraScale) {
  animator->render(renderer, static_cast<int>(mover.position.x),
                   static_cast<int>(mover.position.y), cameraScale);
  SDL_Rect collRect = getHitboxRect();
  SDL_Rect healthBar = {collRect.x, collRect.y - 10, collRect.w, 5};
  float healthRatio = static_cast<float>(health) / maxHealth;

  SDL_Rect healthFill = {healthBar.x, healthBar.y,
                         static_cast<int>(healthBar.w * healthRatio),
                         healthBar.h};

  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  SDL_RenderFillRect(renderer, &healthBar);
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  SDL_RenderFillRect(renderer, &healthFill);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, &healthBar);

  SDL_Rect staminaBar = {collRect.x, collRect.y - 30, collRect.w, 5};
  float staminaRatio = static_cast<float>(stamina) / maxStamina;
  SDL_Rect staminaFill = {staminaBar.x, staminaBar.y,
                          static_cast<int>(staminaBar.w * staminaRatio),
                          staminaBar.h};
  SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
  SDL_RenderFillRect(renderer, &staminaBar);
  SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
  SDL_RenderFillRect(renderer, &staminaFill);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, &staminaBar);
}

void Character::renderWithCamera(SDL_Renderer *renderer, const Camera &camera,
                                 const Config &config) {

  float offsetX = config.windowWidth * 0.5f - camera.position.x * camera.scale;
  float offsetY = config.windowHeight * 0.5f - camera.position.y * camera.scale;

  int renderX = static_cast<int>(offsetX + mover.position.x * camera.scale);
  int renderY = static_cast<int>(offsetY + mover.position.y * camera.scale);

  animator->render(renderer, renderX, renderY, camera.scale);

  SDL_Rect collRect = getHitboxRect();
  collRect.x = static_cast<int>(offsetX + collRect.x * camera.scale);
  collRect.y = static_cast<int>(offsetY + collRect.y * camera.scale);

  SDL_Rect healthBar = {collRect.x, collRect.y - 10, collRect.w, 5};
  float healthRatio = static_cast<float>(health) / maxHealth;
  SDL_Rect healthFill = {healthBar.x, healthBar.y,
                         static_cast<int>(healthBar.w * healthRatio),
                         healthBar.h};
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  SDL_RenderFillRect(renderer, &healthBar);
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  SDL_RenderFillRect(renderer, &healthFill);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, &healthBar);

  SDL_Rect staminaBar = {collRect.x, collRect.y - 30, collRect.w, 5};
  float staminaRatio = static_cast<float>(stamina) / maxStamina;
  SDL_Rect staminaFill = {staminaBar.x, staminaBar.y,
                          static_cast<int>(staminaBar.w * staminaRatio),
                          staminaBar.h};
  SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
  SDL_RenderFillRect(renderer, &staminaBar);
  SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
  SDL_RenderFillRect(renderer, &staminaFill);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, &staminaBar);
}

void Character::updateJumpAnimation() {

  if (!onGround) {
    float vy = mover.velocity.y;

    const float RISE_FAST = -500.0f;
    const float RISE_SLOW = -200.0f;
    const float FALL_SLOW = 200.0f;
    const float FALL_FAST = 500.0f;

    std::string currentAnim = animator->getCurrentAnimationKey();

    if (vy < RISE_FAST) {

      animator->play("Jump");
      animator->setFrameIndex(0);
    } else if (vy < RISE_SLOW) {

      animator->play("Jump");
      animator->setFrameIndex(2);
    } else if (vy > FALL_FAST) {

      animator->play("Jump");
      animator->setFrameIndex(4);
    } else if (vy > FALL_SLOW) {

      animator->play("Jump");
      animator->setFrameIndex(3);
    } else {

      animator->play("Jump");
      animator->setFrameIndex(2);
    }
  } else if (animator->getCurrentAnimationKey() == "Jump") {

    animator->play("Landing");
  }
}

void Character::updateFacing(const Character &target) {
  SDL_Rect myRect = getHitboxRect();
  SDL_Rect targetRect = target.getHitboxRect();
  int myCenterX = myRect.x + myRect.w / 2;
  int targetCenterX = targetRect.x + targetRect.w / 2;
  animator->setFlip(targetCenterX < myCenterX);
  int forwardDirection = (targetCenterX > myCenterX) ? 1 : -1;
  if (isMoving) {
    if (inputDirection == forwardDirection) {
      animator->setReverse(false);
      Logger::debug("Playing walk animation normally (forward).");
    } else {
      animator->setReverse(true);
      Logger::debug("Playing walk animation in reverse (backward).");
    }
  } else {
    animator->setReverse(false);
  }
}
