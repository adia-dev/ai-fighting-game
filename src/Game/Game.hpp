#pragma once
#include "AI/RLAgent.hpp"
#include "Core/Config.hpp"
#include "Core/GuiContext.hpp"
#include "Core/SDLContext.hpp"
#include "Game/Character.hpp"
#include "Game/CharacterControl.hpp"
#include "Game/CombatSystem.hpp"
#include "Game/FightSystem.hpp"
#include "Rendering/Renderer.hpp"
#include "Rendering/Text.hpp"
#include "Rendering/VFX.hpp"
#include "Rendering/Window.hpp"
#include "Resources/ResourceManager.hpp"
#include <Rendering/Animator.hpp>
#include <Rendering/Camera.hpp>
#include <memory>

class Game {
public:
  Game();
  void run();

  void setHeadlessMode(bool enabled) {
    m_headlessMode = enabled;
    if (enabled) {
      m_combatSystem->setTrainingMode(true);
    }
  }

  bool isHeadlessMode() const { return m_headlessMode; }

private:
  void single_iter(void *arg);

  void init();
  void initWindow();
  void initRenderer();
  void initResourceManager();
  void initBackground();
  void initAnimations();
  void initCharacters();
  void initCamera();

  void processInput();

  void update(float deltaTime);
  void updateCamera(float deltaTime);
  void updateCharacterControl(CharacterControl &control, RLAgent *agent,
                              Character *character);
  void handleEnemyInput();

  void render();
  void renderBackground();
  void renderDebugUI();
  void renderPerformanceWindow();
  void renderAIDebugWindow();
  void renderConfigEditor();
  void renderTrainingOverlay();

  void updateScreenEffects(float deltaTime);
  void triggerScreenShake(float duration, float intensity);
  void triggerSlowMotion(float duration, float timeScale);

  void setRoundEnd(const std::string &winnerText);

  Config m_config;

  SDLContext m_sdlContext;
  std::unique_ptr<Window> m_window;
  std::unique_ptr<Renderer> m_renderer;
  std::unique_ptr<ResourceManager> m_resourceManager;
  std::shared_ptr<Texture2D> m_backgroundTexture;

  std::unique_ptr<Animator> m_animatorPlayer;
  std::unique_ptr<Animator> m_animatorEnemy;
  std::unique_ptr<Character> m_player;
  std::unique_ptr<Character> m_enemy;
  std::unique_ptr<RLAgent> m_enemy_agent;
  std::unique_ptr<RLAgent> m_player_agent;
  std::unique_ptr<GuiContext> m_imguiContext;
  std::unique_ptr<CombatSystem> m_combatSystem;

  CharacterControl m_playerControl{"Player"};
  CharacterControl m_enemyControl{"Enemy"};

  float m_deltaTime = 0.0f;
  float m_timeScale = 1.0f;

  bool m_roundEnded = false;
  std::string m_winnerText;
  float m_roundEndTimer = 0.0f;
  float m_zoomEffect = 1.0f;

  Uint64 m_lastCounter = 0.0f;

  bool m_headlessMode = false;
  bool m_showDebugWindow = true;
  bool m_showPerformance = true;
  bool m_showAIDebug = true;
  bool m_showDebugUI = false;
  bool m_showGameView = true;
  bool m_showConfigEditor = true;
  bool m_paused = false;

  float m_trainingRenderTimer = 0.0f;
  const float TRAINING_RENDER_INTERVAL = 0.1f;

  FightSystem m_fightSystem;
  Camera m_camera;
  ScreenShake m_screenShake;
  SlowMotion m_slowMotion;

  static constexpr int MAX_TRAINING_STEPS_PER_FRAME = 10;
  static constexpr float TRAINING_TIME_STEP = 1.0f / 60.0f;

  float m_trainingAccumulator = 0.0f;
  int m_totalEpisodes = 0;
  int m_trainingEpochLength = 100;
};
