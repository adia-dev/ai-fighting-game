#include "Game.hpp"
#include "Animation.hpp"
#include "PiksyAnimationLoader.hpp"
#include "r.hpp"
#include <SDL.h>
#include <iostream>

static const int WINDOW_WIDTH = 800;
static const int WINDOW_HEIGHT = 600;

template <typename T> T clamp(T value, T min, T max) {
  return (value < min) ? min : (value > max ? max : value);
}

bool checkCollision(const SDL_Rect &a, const SDL_Rect &b);
void resolveCollision(Character &a, Character &b);
void applyCollisionImpulse(Character &a, Character &b);

Game::Game() {
  m_window = std::make_unique<Window>("Controllable Game", WINDOW_WIDTH,
                                      WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  m_renderer =
      std::make_unique<Renderer>(m_window->get(), SDL_RENDERER_ACCELERATED);
  m_resourceManager = std::make_unique<ResourceManager>(m_renderer->get());

  auto texture = m_resourceManager->getTexture(R::texture("alex.png"));

  m_animatorPlayer = std::make_unique<Animator>(texture->get());
  m_animatorEnemy = std::make_unique<Animator>(texture->get());

  Animation walkAnimation;
  try {
    walkAnimation =
        PiksyAnimationLoader::loadAnimation(R::animation("Walk.json"));
  } catch (const std::exception &e) {
    std::cerr << "Failed to load animation: " << e.what() << "\n";
    Frame frame;
    frame.frameRect = {0, 2203, 116, 120};
    frame.flipped = false;
    frame.duration_ms = 100;
    walkAnimation.name = "Walk";
    walkAnimation.loop = true;
    walkAnimation.frames.push_back(frame);
  }
  m_animatorPlayer->setAnimation(walkAnimation);
  m_animatorEnemy->setAnimation(walkAnimation);

  Animation idleAnimation = walkAnimation;
  if (!idleAnimation.frames.empty()) {

    idleAnimation.frames.resize(1);
    idleAnimation.loop = false;
  }

  m_player = std::make_unique<Character>(m_animatorPlayer.get());
  m_enemy = std::make_unique<Character>(m_animatorEnemy.get());

  m_player->walkAnimation = walkAnimation;
  m_player->idleAnimation = idleAnimation;
  m_enemy->walkAnimation = walkAnimation;
  m_enemy->idleAnimation = idleAnimation;

  m_player->mover.position = Vector2f(100, 100);
  m_enemy->mover.position = Vector2f(400, 100);
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
    render();
    SDL_Delay(16);
  }
}

void Game::processInput() { m_player->handleInput(); }

void Game::update(float deltaTime) {

  Vector2f toPlayer = m_player->mover.position - m_enemy->mover.position;
  if (toPlayer.length() > 0.0f) {
    Vector2f force = toPlayer.normalized() * 200.0f;
    m_enemy->mover.applyForce(force);
  }

  m_player->update(deltaTime);
  m_enemy->update(deltaTime);

  m_player->updateFacing(*m_enemy);
  m_enemy->updateFacing(*m_player);

  auto clampCharacter = [](Character &ch) {
    SDL_Rect r = ch.animator->getCurrentFrameRect();
    ch.mover.position.x =
        clamp(ch.mover.position.x, 0.f, float(WINDOW_WIDTH - r.w));
    ch.mover.position.y =
        clamp(ch.mover.position.y, 0.f, float(WINDOW_HEIGHT - r.h));
    if (ch.mover.position.y > GROUND_LEVEL)
      ch.mover.position.y = GROUND_LEVEL;
  };
  clampCharacter(*m_player);
  clampCharacter(*m_enemy);

  if (checkCollision(m_player->getCollisionRect(),
                     m_enemy->getCollisionRect())) {
    resolveCollision(*m_player, *m_enemy);
    applyCollisionImpulse(*m_player, *m_enemy);
    m_player->applyDamage(1);
    m_enemy->applyDamage(1);
  }
}

void Game::render() {
  SDL_SetRenderDrawColor(m_renderer->get(), 50, 50, 50, 255);
  SDL_RenderClear(m_renderer->get());

  SDL_SetRenderDrawColor(m_renderer->get(), 100, 255, 100, 255);
  SDL_RenderDrawLine(m_renderer->get(), 0, GROUND_LEVEL, WINDOW_WIDTH,
                     GROUND_LEVEL);

  m_player->render(m_renderer->get());
  m_enemy->render(m_renderer->get());

  SDL_RenderPresent(m_renderer->get());
}

bool checkCollision(const SDL_Rect &a, const SDL_Rect &b) {
  return SDL_HasIntersection(&a, &b);
}

void resolveCollision(Character &a, Character &b) {
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

void applyCollisionImpulse(Character &a, Character &b) {
  SDL_Rect rectA = a.getCollisionRect();
  SDL_Rect rectB = b.getCollisionRect();

  Vector2f centerA(rectA.x + rectA.w / 2.0f, rectA.y + rectA.h / 2.0f);
  Vector2f centerB(rectB.x + rectB.w / 2.0f, rectB.y + rectB.h / 2.0f);

  Vector2f collisionNormal = (centerB - centerA).normalized();
  float impulseStrength = 500.0f;

  a.mover.applyForce(collisionNormal * -impulseStrength);
  b.mover.applyForce(collisionNormal * impulseStrength);
}
