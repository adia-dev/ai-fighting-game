#include "Game.hpp"
#include "Core/Logger.hpp"
#include "Core/Maths.hpp"
#include "Data/Animation.hpp"
#include "Game/CollisionSystem.hpp"
#include "Rendering/Text.hpp"
#include "Resources/PiksyAnimationLoader.hpp"
#include "Resources/R.hpp"
#include <SDL.h>
#include <map>
#include <memory>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

void Game::initWindow() {
  m_window = std::make_unique<Window>("Controllable Game", m_config.windowWidth,
                                      m_config.windowHeight, SDL_WINDOW_SHOWN);
  Logger::info("Window created successfully.");
}

void Game::initRenderer() {
  m_renderer =
      std::make_unique<Renderer>(m_window->get(), SDL_RENDERER_ACCELERATED);
  Logger::info("Renderer initialized.");
}

void Game::initResourceManager() {
  m_resourceManager = std::make_unique<ResourceManager>(m_renderer->get());
  Logger::info("Resource manager initialized.");
}

void Game::initAnimations() {
  auto texture = m_resourceManager->getTexture(R::texture("alex.png"));

  std::map<std::string, Animation> loadedAnimations;
  try {
    loadedAnimations =
        PiksyAnimationLoader::loadAnimation(R::animation("alex.json"));
    Logger::info("Animations loaded successfully.");
  } catch (const std::exception &e) {
    Logger::error("Failed to load animations: " + std::string(e.what()));
  }

  m_animatorPlayer =
      std::make_unique<Animator>(texture->get(), loadedAnimations);
  m_animatorEnemy =
      std::make_unique<Animator>(texture->get(), loadedAnimations);

  m_animatorPlayer->play("Idle");
  m_animatorEnemy->play("Idle");
}

void Game::initCharacters() {
  m_player = std::make_unique<Character>(m_animatorPlayer.get());
  m_enemy = std::make_unique<Character>(m_animatorEnemy.get());

  m_enemy_agent = std::make_unique<RLAgent>(m_enemy.get());
  m_player_agent = std::make_unique<RLAgent>(m_player.get());

  m_player->mover.position = Vector2f(100, 100);
  m_enemy->mover.position = Vector2f(400, 100);
}

void Game::initCamera() {
  m_camera.position =
      (m_player->mover.position + m_enemy->mover.position) * 0.5f;
  m_camera.targetPosition = m_camera.position;
  m_camera.scale = 1.0f;
  m_camera.targetScale = 1.0f;
}

Game::Game() {
  Logger::init();
  initWindow();
  initRenderer();
  initResourceManager();
  initAnimations();
  initCharacters();
  initCamera();
}

void Game::run() {
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(
      [](void *arg) {
        Game *game = static_cast<Game *>(arg);
        game->single_iter(arg);
      },
      this, 0, 1);
#endif

  bool quit = false;
  m_lastCounter = SDL_GetPerformanceCounter();

  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        quit = true;

      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
          m_headlessMode = false;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_TAB) {
          m_headlessMode = true;
        }
      }
    }

    Uint64 currentCounter = SDL_GetPerformanceCounter();
    m_deltaTime = static_cast<float>(currentCounter - m_lastCounter) /
                  SDL_GetPerformanceFrequency();
    m_lastCounter = currentCounter;

    processInput();
    for (size_t i = 0; i < (m_headlessMode ? 100 : 1); ++i) {
      update(m_deltaTime);
    }
    updateCamera(m_deltaTime);
    render();
    SDL_Delay(16);
  }
}

void Game::single_iter(void *arg) {
  Game *game = static_cast<Game *>(arg);

  // Use the persistent m_lastTime or m_lastCounter
  Uint64 currentCounter = SDL_GetPerformanceCounter();
  game->m_deltaTime = static_cast<float>(currentCounter - game->m_lastCounter) /
                      SDL_GetPerformanceFrequency();
  game->m_lastCounter = currentCounter;

  game->processInput();
  game->update(m_deltaTime);
  game->updateCamera(m_deltaTime);
  game->render();
}

void Game::processInput() { m_player->handleInput(); }

void Game::update(float deltaTime) {
  m_combatSystem.update(deltaTime, *m_player, *m_enemy);

  if (m_combatSystem.isRoundActive()) {
    m_enemy_agent->update(deltaTime, *m_player);
    m_player_agent->update(deltaTime, *m_enemy);

    m_player->update(deltaTime);
    m_enemy->update(deltaTime);

    m_player->updateFacing(*m_enemy);
    m_enemy->updateFacing(*m_player);

    // Clamp characters inside screen bounds...
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
      Logger::debug("Player hit enemy!");
    }

    hitLanded = m_fightSystem.processHit(*m_enemy, *m_player);
    if (hitLanded) {
      m_player->applyDamage(1);
      Logger::debug("Enemy hit player!");
    }

    if (CollisionSystem::checkCollision(m_player->getHitboxRect(),
                                        m_enemy->getHitboxRect())) {
      CollisionSystem::resolveCollision(*m_player, *m_enemy);
      CollisionSystem::applyCollisionImpulse(*m_player, *m_enemy,
                                             m_config.defaultMoveForce);
    }
  } else {
    m_combatSystem.startNewRound(*m_player, *m_enemy);
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

    SDL_Rect collRect = ch.getHitboxRect();
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
  m_combatSystem.render(m_renderer->get());

  SDL_Color white = {255, 255, 255, 255};
  drawText(m_renderer->get(), "Agent State:", 10, 350, white);
  drawText(m_renderer->get(), "Health: " + std::to_string(m_player->health), 10,
           370, white);
  drawText(m_renderer->get(), "Delta time: " + std::to_string(m_deltaTime), 500,
           200, white);

  drawText(m_renderer->get(),
           "Last Action: " + std::string(actionTypeToString(
                                 m_player_agent->lastAction().type)),
           10, 390, white);
  drawText(m_renderer->get(),
           "Reward: " + std::to_string(m_player_agent->totalReward()), 10, 410,
           white);

  SDL_RenderPresent(m_renderer->get());
}
