#include "Game.hpp"
#include "AI/NeuralNetworkTreeView.hpp"
#include "AI/NeuralNetworkVisualizer.hpp"
#include "Core/DebugDraw.hpp"
#include "Core/DebugEvents.hpp"
#include "Core/DebugGlobals.hpp"
#include "Core/GuiContext.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Core/Maths.hpp"
#include "Data/Animation.hpp"
#include "Game/CollisionSystem.hpp"
#include "Rendering/ConfigEditor.hpp"
#include "Rendering/DebugOverlay.hpp"
#include "Rendering/Text.hpp"
#include "Resources/PiksyAnimationLoader.hpp"
#include "Resources/R.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <SDL.h>
#include <map>
#include <memory>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

void Game::init() {
  Logger::init();
  initWindow();
  initRenderer();
  initResourceManager();
  initBackground();
  initAnimations();
  initCharacters();
  initCamera();

  m_combatSystem = std::make_unique<CombatSystem>(
      m_config, m_player_agent.get(), m_enemy_agent.get());

  static GuiContext::Config config;
  config.iniFilename = R::config("game_imgui.ini");
  m_imguiContext = std::make_unique<GuiContext>();
  m_imguiContext->init(m_window->get(), m_renderer->get(), config);
}

void Game::initWindow() {
  m_window = std::make_unique<Window>("Controllable Game", m_config.windowWidth,
                                      m_config.windowHeight,
                                      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  Logger::info("Window created successfully.");
}

void Game::initRenderer() {
  m_renderer = std::make_unique<Renderer>(
      m_window->get(), SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  Logger::info("Renderer initialized.");
}

void Game::initResourceManager() {
  m_resourceManager = std::make_unique<ResourceManager>(m_renderer->get());
  Logger::info("Resource manager initialized.");
}

void Game::initBackground() {
  m_backgroundTexture =
      m_resourceManager->getTexture(R::texture("the_grid.jpeg"));
  Logger::info("Background initialized.");
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
  m_player = std::make_unique<Character>(m_animatorPlayer.get(), m_config);
  m_enemy = std::make_unique<Character>(m_animatorEnemy.get(), m_config);

  m_enemy_agent = std::make_unique<RLAgent>(m_enemy.get(), m_config);
  m_player_agent = std::make_unique<RLAgent>(m_player.get(), m_config);

  SDL_Rect playerRect = m_player->animator->getCurrentFrameRect();
  SDL_Rect enemyRect = m_enemy->animator->getCurrentFrameRect();

  float playerY = m_config.groundLevel - playerRect.h;
  float enemyY = m_config.groundLevel - enemyRect.h;

  m_player->mover.position = Vector2f(200, playerY);
  m_enemy->mover.position = Vector2f(600, enemyY);
}

void Game::initCamera() {
  m_camera.position =
      (m_player->mover.position + m_enemy->mover.position) * 0.5f;
  m_camera.targetPosition = m_camera.position;
  m_camera.scale = 1.0f;
  m_camera.targetScale = 1.0f;
}

Game::Game() { init(); }

void Game::run() {
#ifdef __EMSCRIPTEN__
  m_lastCounter = SDL_GetPerformanceCounter();

  static Game *gameInstance = this;
  emscripten_set_main_loop_arg(
      [](void *arg) {
        Game *game = static_cast<Game *>(arg);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
          game->m_imguiContext->processEvent(event);

          if (event.type == SDL_QUIT)
            emscripten_cancel_main_loop();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
          game->m_showDebugUI = false;
        } else if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
          game->m_showDebugUI = !game->m_showDebugUI;
        }

        Uint64 currentCounter = SDL_GetPerformanceCounter();
        game->m_deltaTime =
            static_cast<float>(currentCounter - game->m_lastCounter) /
            SDL_GetPerformanceFrequency();
        game->m_lastCounter = currentCounter;

        game->m_imguiContext->beginFrame();

        game->processInput();
        game->update(game->m_deltaTime);
        game->updateCamera(game->m_deltaTime);

        game->render();

        game->renderDebugUI();

        game->m_imguiContext->endFrame();
      },
      this, 0, 1);
#else
  bool quit = false;
  m_lastCounter = SDL_GetPerformanceCounter();

  while (!quit) {
    if (!m_headlessMode) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        m_imguiContext->processEvent(event);
        if (event.type == SDL_QUIT)
          quit = true;
      }

      if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_showDebugUI = false;
      } else if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        m_showDebugUI = !m_showDebugUI;
      }
    }

    Uint64 currentCounter = SDL_GetPerformanceCounter();
    m_deltaTime = static_cast<float>(currentCounter - m_lastCounter) /
                  SDL_GetPerformanceFrequency();
    m_lastCounter = currentCounter;

    if (!m_headlessMode) {
      m_imguiContext->beginFrame();
    }

    processInput();

    // Training mode improvements
    if (m_combatSystem->trainingMode()) {
      m_trainingAccumulator += m_deltaTime;

      int stepsThisFrame = 0;
      while (m_trainingAccumulator >= TRAINING_TIME_STEP &&
             stepsThisFrame < MAX_TRAINING_STEPS_PER_FRAME) {

        update(TRAINING_TIME_STEP);
        m_trainingAccumulator -= TRAINING_TIME_STEP;
        stepsThisFrame++;

        // Check if episode ended
        if (!m_combatSystem->isRoundActive()) {
          m_totalEpisodes++;

          // Every m_trainingEpochLength episodes, perform model updates
          if (m_totalEpisodes % m_trainingEpochLength == 0) {
            if (m_player_agent)
              m_player_agent->updateTargetNetwork();
            if (m_enemy_agent)
              m_enemy_agent->updateTargetNetwork();
            Logger::info("Training epoch completed. Episodes: " +
                         std::to_string(m_totalEpisodes));
          }
        }
      }
    } else {
      update(m_deltaTime);
    }

    if (!m_headlessMode) {
      updateCamera(m_deltaTime);
      render();
      renderDebugUI();
      m_imguiContext->endFrame();
    }
  }
#endif
}

void Game::single_iter(void *arg) {
  Game *game = static_cast<Game *>(arg);

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
  if (m_paused)
    return;

  deltaTime *= m_timeScale;

  m_combatSystem->update(deltaTime, *m_player, *m_enemy);

  if (m_combatSystem->isRoundActive()) {
    m_fightSystem.update(deltaTime);

    if (m_playerControl.enabled) {
      switch (m_playerControl.mode) {
      case ControlMode::Human:
        m_player->handleInput();
        break;
      case ControlMode::AI:
        m_player_agent->update(deltaTime, *m_enemy);
        break;
      case ControlMode::Disabled:
        break;
      }
    }

    if (m_enemyControl.enabled) {
      switch (m_enemyControl.mode) {
      case ControlMode::Human:

        handleEnemyInput();
        break;
      case ControlMode::AI:
        m_enemy_agent->update(deltaTime, *m_player);
        break;
      case ControlMode::Disabled:
        break;
      }
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

    if (m_fightSystem.processHit(*m_player, *m_enemy)) {
      m_enemy->applyDamage(1);
      Logger::debug("Player hit enemy!");
    }

    if (m_fightSystem.processHit(*m_enemy, *m_player)) {
      m_player->applyDamage(1);
      Logger::debug("Enemy hit player!");
    }

    if (CollisionSystem::checkCollision(m_player->getHitboxRect(),
                                        m_enemy->getHitboxRect())) {
      CollisionSystem::resolveCollision(*m_player, *m_enemy);
      CollisionSystem::applyCollisionImpulse(*m_player, *m_enemy,
                                             m_config.moveForce);
    }
  } else {
    m_combatSystem->startNewRound(*m_player, *m_enemy);
  }
}

void Game::handleEnemyInput() {
  if (Input::isKeyDown(SDL_SCANCODE_D))
    m_enemy->mover.applyForce(Vector2f(-m_config.moveForce, 0));
  if (Input::isKeyDown(SDL_SCANCODE_G))
    m_enemy->mover.applyForce(Vector2f(m_config.moveForce, 0));
  if (Input::isKeyDown(SDL_SCANCODE_R))
    m_enemy->attack();
  if (Input::isKeyDown(SDL_SCANCODE_T))
    m_enemy->block();
  if (Input::isKeyDown(SDL_SCANCODE_F) && m_enemy->onGround &&
      m_enemy->groundFrames >= m_config.stableGroundFrames) {
    m_enemy->jump();
  }
}

void Game::updateCamera(float deltaTime) {

  Vector2f midpoint =
      (m_player->mover.position + m_enemy->mover.position) * 0.5f;

  float groundOffset = (m_config.groundLevel - midpoint.y) * 0.2f;
  midpoint.y += groundOffset;

  float dist = (m_player->mover.position - m_enemy->mover.position).length();

  float desiredZoom = m_camera.defaultZoom;
  if (dist > m_camera.focusMarginX * 2) {
    float zoomFactor =
        (dist - m_camera.focusMarginX * 2) / m_camera.focusMarginX;
    desiredZoom = m_camera.defaultZoom - (zoomFactor * 0.2f);
    desiredZoom = std::max(desiredZoom, m_camera.minZoom);
  }

  m_camera.targetScale = desiredZoom;
  m_camera.scale = lerp(m_camera.scale, m_camera.targetScale,
                        deltaTime * m_camera.zoomSpeed);

  Vector2f targetPos = midpoint;

  targetPos.x = clamp(
      targetPos.x,
      m_camera.boundaryLeft + m_config.windowWidth * 0.5f / m_camera.scale,
      m_camera.boundaryRight - m_config.windowWidth * 0.5f / m_camera.scale);

  targetPos.y = clamp(
      targetPos.y,
      m_camera.boundaryTop + m_config.windowHeight * 0.5f / m_camera.scale,
      m_camera.boundaryBottom - m_config.windowHeight * 0.5f / m_camera.scale);

  m_camera.targetPosition = targetPos;
  m_camera.position = lerp(m_camera.position, m_camera.targetPosition,
                           deltaTime * m_camera.moveSpeed);
}

void Game::render() {
  if (m_headlessMode)
    return;

  if (m_combatSystem->trainingMode()) {
    m_trainingRenderTimer += m_deltaTime;
    if (m_trainingRenderTimer < TRAINING_RENDER_INTERVAL) {
      return;
    }
    m_trainingRenderTimer = 0.0f;
  }

  Vector2f originalPos = m_camera.position;
  m_camera.position += m_screenShake.offset;

  SDL_SetRenderDrawColor(m_renderer->get(), 50, 50, 50, 255);
  SDL_RenderClear(m_renderer->get());

  renderBackground();

  Vector2f offset;
  offset.x = m_config.windowWidth * 0.5f - m_camera.position.x * m_camera.scale;
  offset.y =
      m_config.windowHeight * 0.5f - m_camera.position.y * m_camera.scale;

  ImDrawList *draw_list = ImGui::GetBackgroundDrawList();

  for (auto it = g_damageEvents.begin(); it != g_damageEvents.end();) {

    float screenX = offset.x + it->position.x * m_camera.scale;
    float screenY = offset.y + it->position.y * m_camera.scale;

    int alpha = static_cast<int>((it->timeRemaining / 1.0f) * 255);
    ImU32 col = IM_COL32(255, 0, 0, alpha);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", it->damage);
    draw_list->AddText(ImVec2(screenX, screenY), col, buf);

    it->position.y -= 20.0f * m_deltaTime;
    it->timeRemaining -= m_deltaTime;
    if (it->timeRemaining <= 0)
      it = g_damageEvents.erase(it);
    else
      ++it;
  }

  SDL_SetRenderDrawColor(m_renderer->get(), 100, 255, 100, 255);
  SDL_RenderDrawLine(m_renderer->get(), 0, m_config.groundLevel,
                     m_config.windowWidth, m_config.groundLevel);

  m_enemy->renderWithCamera(m_renderer->get(), m_camera, m_config);
  m_player->renderWithCamera(m_renderer->get(), m_camera, m_config);
  m_combatSystem->render(m_renderer->get());

  if (g_showDebugOverlay) {
    DebugOverlay::renderGameZones(m_renderer->get(), m_camera, m_config);
    DebugOverlay::renderCharacterInfo(m_renderer->get(), *m_player, m_camera,
                                      m_config);
    DebugOverlay::renderCharacterInfo(m_renderer->get(), *m_enemy, m_camera,
                                      m_config);
  }

  if (m_roundEnded) {

    m_roundEndTimer += m_deltaTime;

    m_zoomEffect = 1.0f + m_roundEndTimer * 0.5f;

    SDL_Color textColor = {255, 255, 255, 255};

    drawCenteredText(m_renderer->get(), m_winnerText, m_config.windowWidth / 2,
                     m_config.windowHeight / 2, textColor, m_zoomEffect);

    if (m_roundEndTimer >= 3.0f) {
      m_combatSystem->startNewRound(*m_player, *m_enemy);
      m_roundEnded = false;
    }
  }

  m_camera.position = originalPos;

  renderTrainingOverlay();
}

void Game::renderBackground() {

  SDL_Rect bgRect = {0, 0, m_config.windowWidth, m_config.windowHeight};
  SDL_RenderCopy(m_renderer->get(), m_backgroundTexture->get(), nullptr,
                 &bgRect);

  int groundY = m_config.groundLevel;
  SDL_SetRenderDrawColor(m_renderer->get(), 100, 100, 100, 255);

  int gridSpacing = 50;
  int numLines = m_config.windowWidth / gridSpacing;

  for (int i = 0; i <= numLines; i++) {
    int x = i * gridSpacing;
    SDL_RenderDrawLine(m_renderer->get(), x, groundY, x, groundY + 20);
  }

  SDL_SetRenderDrawColor(m_renderer->get(), 150, 150, 150, 255);
  SDL_RenderDrawLine(m_renderer->get(), 0, groundY, m_config.windowWidth,
                     groundY);
}

void Game::renderDebugUI() {
  if (!m_showDebugUI)
    return;

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                  ImGuiWindowFlags_NoBringToFrontOnFocus |
                  ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", nullptr, window_flags);
  ImGui::PopStyleVar(3);

  ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Windows")) {
      ImGui::MenuItem("Game View", nullptr, &m_showGameView);
      ImGui::MenuItem("Debug Controls", nullptr, &m_showDebugWindow);
      ImGui::MenuItem("AI Debug", nullptr, &m_showAIDebug);
      ImGui::MenuItem("Performance", nullptr, &m_showPerformance);
      ImGui::MenuItem("Config Editor", nullptr, &m_showConfigEditor);
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  ImGui::End();

  if (m_showGameView) {
    ImGui::Begin("Game View", &m_showGameView);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    float dpiScale = m_imguiContext->getDpiScale();

    m_imguiContext->createGameViewport(static_cast<int>(viewportSize.x),
                                       static_cast<int>(viewportSize.y));

    m_imguiContext->beginGameViewportRender();

    SDL_RenderSetScale(m_renderer->get(), dpiScale, dpiScale);
    render();
    SDL_RenderSetScale(m_renderer->get(), 1.0f, 1.0f);

    m_imguiContext->endGameViewportRender();

    ImGui::Image(
        (ImTextureID)(uintptr_t)m_imguiContext->getGameViewportTexture(),
        viewportSize);

    if (ImGui::IsWindowHovered()) {
      ImVec2 mousePos = ImGui::GetMousePos();
      ImVec2 windowPos = ImGui::GetWindowPos();
      ImVec2 relativePos =
          ImVec2(mousePos.x - windowPos.x, mousePos.y - windowPos.y);

      relativePos.x *= dpiScale;
      relativePos.y *= dpiScale;
    }

    ImGui::End();
  }

  if (m_showAIDebug) {
    renderAIDebugWindow();
  }
  if (m_showPerformance) {
    renderPerformanceWindow();
  }
  if (m_showConfigEditor) {
    ConfigEditor::render(*this, m_config, m_showConfigEditor);
  }
}

void Game::renderPerformanceWindow() {
  ImGui::Begin("Performance", &m_showPerformance);

  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

  static float values[90] = {0};
  static int values_offset = 0;

  values[values_offset] = ImGui::GetIO().Framerate;
  values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);

  ImGui::PlotLines("FPS", values, IM_ARRAYSIZE(values), values_offset, nullptr,
                   0.0f, 120.0f, ImVec2(0, 80));

  ImGui::End();
}

void Game::renderAIDebugWindow() {
  if (!m_showAIDebug)
    return;

  ImGui::Begin("AI Control & Debug", &m_showAIDebug,
               ImGuiWindowFlags_NoCollapse);

  DebugDraw::DrawAIState(*m_player_agent);

  static NeuralNetworkVisualizer *nnVisualizer = nullptr;

  if (ImGui::CollapsingHeader("Neural Network Visualizer")) {
    if (nnVisualizer == nullptr) {
      nnVisualizer =
          new NeuralNetworkVisualizer(m_player_agent->onlineDQN.get());
    }
    nnVisualizer->render();
  }

  if (ImGui::CollapsingHeader("Neural Network Tree View")) {
    static NeuralNetworkTreeView treeView(m_player_agent->onlineDQN.get());
    treeView.render();
  }

  if (ImGui::CollapsingHeader("Global Controls",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Indent();

    if (ImGui::Checkbox("Pause Game", &m_paused)) {
    }
    if (ImGui::Checkbox("Training Mode", &m_combatSystem->trainingMode())) {
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Toggle game pause. When paused, simulation stops.");

    if (ImGui::SliderFloat("Time Scale", &m_timeScale, 0.1f, 50.0f, "%.1fx")) {
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Adjust the simulation speed. 1.0 = normal speed.");

    ImGui::Checkbox("Show Config Editor", &m_showConfigEditor);

    if (ImGui::CollapsingHeader("Debug Visualization")) {
      ImGui::Checkbox("Show Debug Overlay", &g_showDebugOverlay);
    }

    if (ImGui::IsItemHovered())
      ImGui::SetTooltip(
          "Toggle rendering of character hitboxes for debugging.");

    ImGui::Checkbox("Show Floating Damage", &g_showFloatingDamage);
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Toggle floating damage text when characters are hit.");

    ImGui::Unindent();
  }

  auto renderCharacterControls =
      [this](const char *charId, CharacterControl &control, RLAgent *agent) {
        ImGui::PushID(charId);

        if (ImGui::CollapsingHeader(control.name.c_str())) {
          ImGui::Indent();

          const char *modes[] = {"Human", "AI", "Disabled"};
          int currentMode = static_cast<int>(control.mode);
          if (ImGui::Combo("Control Mode", &currentMode, modes,
                           IM_ARRAYSIZE(modes))) {
            control.mode = static_cast<ControlMode>(currentMode);
          }

          if (control.mode == ControlMode::AI && agent) {

            ImGui::Separator();
            ImGui::Text("AI Parameters");

            ImGui::SliderFloat("Exploration (ε)", &control.epsilon, 0.0f, 1.0f,
                               "%.3f");
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("Higher values encourage more random actions");
            }

            ImGui::SliderFloat("Learning Rate", &control.learningRate, 0.0001f,
                               0.01f, "%.4f");
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("How quickly the AI adapts to new experiences");
            }

            ImGui::SliderFloat("Discount (γ)", &control.discountFactor, 0.8f,
                               0.99f, "%.3f");
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip(
                  "Importance of future rewards vs immediate rewards");
            }

            ImGui::Separator();
            ImGui::Text("Training Statistics");

            ImGui::BeginTable("stats", 2, ImGuiTableFlags_Borders);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Episodes");
            ImGui::TableNextColumn();
            ImGui::Text("%d", agent->getEpisodeCount());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Win Rate");
            ImGui::TableNextColumn();
            ImGui::Text("%.1f%%", agent->getWinRate() * 100.0f);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Current Reward");
            ImGui::TableNextColumn();
            ImGui::Text("%.2f", agent->totalReward());
            ImGui::EndTable();

            if (ImGui::Button("Reset Agent")) {
              agent->reset();
            }

            ImGui::Separator();
            static std::unordered_map<std::string, std::vector<float>>
                rewardHistories;
            static std::unordered_map<std::string, int> rewardOffsets;
            std::string idStr = charId;
            if (rewardHistories.find(idStr) == rewardHistories.end()) {
              rewardHistories[idStr] = std::vector<float>(100, 0.0f);
              rewardOffsets[idStr] = 0;
            }
            std::vector<float> &rewardHistory = rewardHistories[idStr];
            int &offset = rewardOffsets[idStr];

            rewardHistory[offset] = agent->totalReward();
            offset = (offset + 1) % rewardHistory.size();

            float minReward =
                *std::min_element(rewardHistory.begin(), rewardHistory.end());
            float maxReward =
                *std::max_element(rewardHistory.begin(), rewardHistory.end());
            float graphMin = std::min(-100.0f, minReward);
            float graphMax = std::max(100.0f, maxReward);

            ImGui::PlotLines("Reward History", rewardHistory.data(),
                             rewardHistory.size(), offset, NULL, graphMin,
                             graphMax, ImVec2(0, 80));

            ImGui::Separator();
            const State &state = agent->getCurrentState();

            ImGui::Text("State Information");
            float radius = 50.0f;
            ImVec2 center = ImGui::GetCursorScreenPos();
            center.x += radius;
            center.y += radius;

            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            for (int i = 0; i < 4; i++) {
              float angle = i * IM_PI / 2.0f;
              float value = state.radar[i] / 400.0f;
              ImVec2 point(center.x + cosf(angle) * radius * value,
                           center.y + sinf(angle) * radius * value);
              draw_list->AddLine(center, point, IM_COL32(0, 255, 0, 255));
            }

            ImGui::Dummy(ImVec2(radius * 2, radius * 2));
          }

          ImGui::Unindent();
        }
        ImGui::PopID();
      };

  renderCharacterControls("player", m_playerControl, m_player_agent.get());
  ImGui::Separator();
  renderCharacterControls("enemy", m_enemyControl, m_enemy_agent.get());

  if (m_player_agent) {
    ImGui::Separator();
    ImGui::Text("Battle Style Control");

    static int battleStyleInt = 1;
    if (ImGui::Combo("Battle Style", &battleStyleInt,
                     "Aggressive\0Balanced\0Defensive\0")) {

      BattleStyle bs;
      if (battleStyleInt == 0) {
        bs.timePenalty = 0.008f;
        bs.hpRatioWeight = 1.0f;
        bs.distancePenalty = 0.002f;
      } else if (battleStyleInt == 1) {
        bs.timePenalty = 0.004f;
        bs.hpRatioWeight = 1.0f;
        bs.distancePenalty = 0.0002f;
      } else if (battleStyleInt == 2) {
        bs.timePenalty = 0.0f;
        bs.hpRatioWeight = 1.2f;
        bs.distancePenalty = 0.0f;
      }
      m_player_agent->setBattleStyle(bs);
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip(
          "Select the battle style for the agent. Aggressive: faster, riskier "
          "attacks; Defensive: cautious play; Balanced: a mix of both.");

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    pos.y += 20;
    float width = 300.0f;
    float height = 20.0f;

    draw_list->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                             IM_COL32(100, 100, 100, 255));

    draw_list->AddText(ImVec2(pos.x, pos.y - 15), IM_COL32(255, 0, 0, 255),
                       "Aggressive");
    draw_list->AddText(ImVec2(pos.x + width / 2 - 30, pos.y - 15),
                       IM_COL32(255, 255, 0, 255), "Balanced");
    draw_list->AddText(ImVec2(pos.x + width - 80, pos.y - 15),
                       IM_COL32(0, 255, 0, 255), "Defensive");

    float pointerX = pos.x;
    if (battleStyleInt == 0)
      pointerX = pos.x;
    else if (battleStyleInt == 1)
      pointerX = pos.x + width / 2;
    else if (battleStyleInt == 2)
      pointerX = pos.x + width;

    draw_list->AddLine(ImVec2(pointerX, pos.y),
                       ImVec2(pointerX, pos.y + height),
                       IM_COL32(0, 0, 255, 255), 3.0f);
    ImGui::Dummy(ImVec2(width, height + 10));
  }

  ImGui::End();
}

void Game::updateCharacterControl(CharacterControl &control, RLAgent *agent,
                                  Character *character) {
  if (!control.enabled)
    return;

  switch (control.mode) {
  case ControlMode::Human:
    character->handleInput();
    break;
  case ControlMode::AI:
    if (agent) {
      agent->setParameters(control.epsilon, control.learningRate,
                           control.discountFactor);
    }
    break;
  case ControlMode::Disabled:

    break;
  }
}
void Game::setRoundEnd(const std::string &winnerText) {
  m_roundEnded = true;
  m_winnerText = winnerText;
  m_roundEndTimer = 0.0f;
  m_zoomEffect = 1.0f;
}

void Game::updateScreenEffects(float deltaTime) {

  m_screenShake.update(deltaTime);

  if (m_slowMotion.active) {
    m_slowMotion.currentTime += deltaTime;
    if (m_slowMotion.currentTime >= m_slowMotion.duration) {
      m_slowMotion.active = false;
      m_timeScale = 1.0f;
    }
  }
}

void Game::triggerScreenShake(float duration, float intensity) {
  m_screenShake.duration = duration;
  m_screenShake.intensity = intensity;
  m_screenShake.currentTime = 0.0f;
}

void Game::triggerSlowMotion(float duration, float timeScale) {
  m_slowMotion.duration = duration;
  m_slowMotion.timeScale = timeScale;
  m_slowMotion.currentTime = 0.0f;
  m_slowMotion.active = true;
  m_timeScale = timeScale;
}
void Game::renderTrainingOverlay() {
  if (!m_combatSystem->trainingMode())
    return;

  SDL_SetRenderDrawBlendMode(m_renderer->get(), SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(m_renderer->get(), 0, 0, 0, 230);
  SDL_Rect overlay = {0, 0, m_config.windowWidth, m_config.windowHeight};
  SDL_RenderFillRect(m_renderer->get(), &overlay);

  SDL_Color textColor = {255, 255, 255, 255};
  std::string trainingInfo =
      "Training Mode - Episode: " +
      std::to_string(m_player_agent->getEpisodeCount()) +
      "\nWin Rate: " + std::to_string(m_player_agent->getWinRate() * 100.0f) +
      "%";

  drawCenteredText(m_renderer->get(), trainingInfo, m_config.windowWidth / 2,
                   m_config.windowHeight / 2, textColor, 1.5f);

  SDL_SetRenderDrawBlendMode(m_renderer->get(), SDL_BLENDMODE_NONE);
}
