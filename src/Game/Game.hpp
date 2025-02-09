#pragma once
#include "AI/RLAgent.hpp"
#include "Core/Config.hpp"
#include "Core/SDLContext.hpp"
#include "Game/Character.hpp"
#include "Game/CombatSystem.hpp"
#include "Game/FightSystem.hpp"
#include "Rendering/Renderer.hpp"
#include "Rendering/Window.hpp"
#include "Resources/ResourceManager.hpp"
#include <Rendering/Animator.hpp>
#include <Rendering/Camera.hpp>
#include <memory>

class Game {
public:
  Game();
  void run();

private:
  void single_iter(void);

  void initWindow();
  void initRenderer();
  void initResourceManager();
  void initAnimations();
  void initCharacters();
  void initCamera();

  void processInput();
  void update(float deltaTime);
  void updateCamera(float deltaTime);
  void render();

  // Core configuration.
  Config m_config;

  // RAII wrappers:
  SDLContext m_sdlContext;
  std::unique_ptr<Window> m_window;
  std::unique_ptr<Renderer> m_renderer;
  std::unique_ptr<ResourceManager> m_resourceManager;

  // Game objects:
  std::unique_ptr<Animator> m_animatorPlayer;
  std::unique_ptr<Animator> m_animatorEnemy;
  std::unique_ptr<Character> m_player;
  std::unique_ptr<Character> m_enemy;
  std::unique_ptr<RLAgent> m_enemy_agent;
  std::unique_ptr<RLAgent> m_player_agent;

  bool m_headlessMode = false;

  FightSystem m_fightSystem;
  CombatSystem m_combatSystem;
  Camera m_camera;
};
