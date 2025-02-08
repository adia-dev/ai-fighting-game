#include "Game.hpp"
#include "Core/Maths.hpp"
#include "Data/Animation.hpp"
#include "Resources/PiksyAnimationLoader.hpp"
#include "Resources/R.hpp"
#include <SDL.h>
#include <iostream>
#include <map>
#include <string>

static bool checkCollision(const SDL_Rect &a, const SDL_Rect &b) {
  return SDL_HasIntersection(&a, &b);
}

static void resolveCollision(Character &a, Character &b) {
  SDL_Rect rectA = a.getCollisionRect();
  SDL_Rect rectB = b.getCollisionRect();
  if (!checkCollision(rectA, rectB))
    return;
  SDL_Rect intersection;
  SDL_IntersectRect(&rectA, &rectB, &intersection);
  if (intersection.w < intersection.h) {
    int separation = intersection.w / 2;
    if (rectA.x < rectB.x) {
      a.mover.position.x -= separation;
      b.mover.position.x += separation;
    } else {
      a.mover.position.x += separation;
      b.mover.position.x -= separation;
    }
  } else {
    int separation = intersection.h / 2;
    if (rectA.y < rectB.y) {
      a.mover.position.y -= separation;
      b.mover.position.y += separation;
    } else {
      a.mover.position.y += separation;
      b.mover.position.y -= separation;
    }
  }
}

static void applyCollisionImpulse(Character &a, Character &b,
                                  float impulseStrength) {
  SDL_Rect rectA = a.getCollisionRect();
  SDL_Rect rectB = b.getCollisionRect();
  Vector2f centerA(rectA.x + rectA.w / 2.0f, rectA.y + rectA.h / 2.0f);
  Vector2f centerB(rectB.x + rectB.w / 2.0f, rectB.y + rectB.h / 2.0f);
  Vector2f collisionNormal = (centerB - centerA).normalized();

  a.mover.applyForce(collisionNormal * -impulseStrength);
  b.mover.applyForce(collisionNormal * impulseStrength);
}

Game::Game() {
  m_window = std::make_unique<Window>("Controllable Game", m_config.windowWidth,
                                      m_config.windowHeight, SDL_WINDOW_SHOWN);
  m_renderer =
      std::make_unique<Renderer>(m_window->get(), SDL_RENDERER_ACCELERATED);
  m_resourceManager = std::make_unique<ResourceManager>(m_renderer->get());

  auto texture = m_resourceManager->getTexture(R::texture("alex.png"));

  m_animatorPlayer = std::make_unique<Animator>(texture->get());
  m_animatorEnemy = std::make_unique<Animator>(texture->get());

  std::map<std::string, Animation> anims;
  try {
    anims = PiksyAnimationLoader::loadAnimation(R::animation("alex.json"));
  } catch (const std::exception &e) {
    std::cerr << "Failed to load animations: " << e.what() << "\n";
  }

  if (anims.find("Walk") != anims.end()) {
    m_animatorPlayer->addAnimation("Walk", anims.at("Walk"));
    m_animatorEnemy->addAnimation("Walk", anims.at("Walk"));
  }
  if (anims.find("Attack") != anims.end()) {
    m_animatorPlayer->addAnimation("Attack", anims.at("Attack"));
    m_animatorEnemy->addAnimation("Attack", anims.at("Attack"));
  }
  if (anims.find("Idle") != anims.end()) {
    m_animatorPlayer->addAnimation("Idle", anims.at("Idle"));
    m_animatorEnemy->addAnimation("Idle", anims.at("Idle"));
  } else {
    Animation idleAnim = anims.at("Walk");
    if (!idleAnim.frames.empty()) {
      idleAnim.frames.resize(1);
      idleAnim.loop = false;
    }
    m_animatorPlayer->addAnimation("Idle", idleAnim);
    m_animatorEnemy->addAnimation("Idle", idleAnim);
  }

  m_animatorPlayer->play("Idle");
  m_animatorEnemy->play("Idle");

  m_player = std::make_unique<Character>(m_animatorPlayer.get());
  m_enemy = std::make_unique<Character>(m_animatorEnemy.get());

  m_player->attackAnimation = anims["Attack"];
  m_player->walkAnimation = anims["Walk"];
  m_player->idleAnimation = (anims.find("Idle") != anims.end())
                                ? anims["Idle"]
                                : m_player->walkAnimation;
  m_enemy->walkAnimation = anims["Walk"];
  m_enemy->idleAnimation = (anims.find("Idle") != anims.end())
                               ? anims["Idle"]
                               : m_enemy->walkAnimation;

  m_player->mover.position = Vector2f(100, 100);
  m_enemy->mover.position = Vector2f(400, 100);

  m_camera.position =
      (m_player->mover.position + m_enemy->mover.position) * 0.5f;
  m_camera.targetPosition = m_camera.position;
  m_camera.scale = 1.0f;
  m_camera.targetScale = 1.0f;
}

void Game::run() {
  bool quit = false;
  Uint32 lastTime = SDL_GetTicks();
  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        quit = true;
    }
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.f;
    lastTime = currentTime;

    processInput();
    update(deltaTime);
    updateCamera(deltaTime);
    render();
    SDL_Delay(16);
  }
}

void Game::processInput() { m_player->handleInput(); }

void Game::update(float deltaTime) {

  Vector2f toPlayer = m_player->mover.position - m_enemy->mover.position;
  if (toPlayer.length() > 0.0f) {
    Vector2f force = toPlayer.normalized() * m_config.enemyFollowForce;
    m_enemy->mover.applyForce(force);
  }

  m_player->update(deltaTime);
  m_enemy->update(deltaTime);

  m_player->updateFacing(*m_enemy);
  m_enemy->updateFacing(*m_player);

  auto clampCharacter = [this](Character &ch) {
    SDL_Rect r = ch.animator->getCurrentFrameRect();
    ch.mover.position.x =
        clamp(ch.mover.position.x, 0.f, float(m_config.windowWidth - r.w));
    ch.mover.position.y =
        clamp(ch.mover.position.y, 0.f, float(m_config.windowHeight - r.h));
    if (ch.mover.position.y > m_config.groundLevel)
      ch.mover.position.y = m_config.groundLevel;
  };
  clampCharacter(*m_player);
  clampCharacter(*m_enemy);

  bool hitLanded = m_fightSystem.processHit(*m_player, *m_enemy);
  if (hitLanded) {
    m_enemy->applyDamage(1);
    std::cout << "[DEBUG] Player hit enemy!\n";
  }

  hitLanded = m_fightSystem.processHit(*m_enemy, *m_player);
  if (hitLanded) {
    m_player->applyDamage(1);
    std::cout << "[DEBUG] Enemy hit player!\n";
  }

  if (checkCollision(m_player->getCollisionRect(),
                     m_enemy->getCollisionRect())) {
    resolveCollision(*m_player, *m_enemy);
    applyCollisionImpulse(*m_player, *m_enemy, m_config.defaultMoveForce);
  }
}

void Game::updateCamera(float deltaTime) {

  Vector2f midpoint =
      (m_player->mover.position + m_enemy->mover.position) * 0.5f;
  m_camera.targetPosition = midpoint;

  float dist = (m_player->mover.position - m_enemy->mover.position).length();
  float t = clamp((dist - m_config.minDistance) /
                      (m_config.maxDistance - m_config.minDistance),
                  0.0f, 1.0f);
  m_camera.targetScale =
      m_config.maxZoom + (m_config.minZoom - m_config.maxZoom) * t;

  m_camera.position.x += (m_camera.targetPosition.x - m_camera.position.x) *
                         m_config.cameraSmoothFactor;
  m_camera.position.y += (m_camera.targetPosition.y - m_camera.position.y) *
                         m_config.cameraSmoothFactor;
  m_camera.scale +=
      (m_camera.targetScale - m_camera.scale) * m_config.cameraSmoothFactor;
}

void Game::render() {

  Vector2f offset;
  offset.x = m_config.windowWidth * 0.5f - m_camera.position.x * m_camera.scale;
  offset.y =
      m_config.windowHeight * 0.5f - m_camera.position.y * m_camera.scale;

  SDL_SetRenderDrawColor(m_renderer->get(), 50, 50, 50, 255);
  SDL_RenderClear(m_renderer->get());

  SDL_SetRenderDrawColor(m_renderer->get(), 100, 255, 100, 255);
  SDL_RenderDrawLine(m_renderer->get(), 0, m_config.groundLevel,
                     m_config.windowWidth, m_config.groundLevel);

  auto renderCharacterWithCamera = [&](Character &ch) {
    int renderX =
        static_cast<int>(offset.x + ch.mover.position.x * m_camera.scale);
    int renderY =
        static_cast<int>(offset.y + ch.mover.position.y * m_camera.scale);

    ch.animator->render(m_renderer->get(), renderX, renderY, m_camera.scale);

    SDL_Rect collRect = ch.getCollisionRect();
    collRect.x = static_cast<int>(offset.x + collRect.x * m_camera.scale);
    collRect.y = static_cast<int>(offset.y + collRect.y * m_camera.scale);
    SDL_Rect healthBar = {collRect.x, collRect.y - 10, collRect.w, 5};
    float ratio = static_cast<float>(ch.health) / ch.maxHealth;
    SDL_Rect healthFill = {healthBar.x, healthBar.y,
                           static_cast<int>(healthBar.w * ratio), healthBar.h};
    SDL_SetRenderDrawColor(m_renderer->get(), 255, 0, 0, 255);
    SDL_RenderFillRect(m_renderer->get(), &healthBar);
    SDL_SetRenderDrawColor(m_renderer->get(), 0, 255, 0, 255);
    SDL_RenderFillRect(m_renderer->get(), &healthFill);
    SDL_SetRenderDrawColor(m_renderer->get(), 0, 0, 0, 255);
    SDL_RenderDrawRect(m_renderer->get(), &healthBar);
  };

  renderCharacterWithCamera(*m_player);
  renderCharacterWithCamera(*m_enemy);

  SDL_RenderPresent(m_renderer->get());
}
