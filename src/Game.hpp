#pragma once
#include "Animator.hpp"
#include "Character.hpp"
#include "Renderer.hpp"
#include "ResourceManager.hpp"
#include "SDLContext.hpp"
#include "Vector2f.hpp"
#include "Window.hpp"
#include <memory>

class Game {
public:
  Game();
  void run();

private:
  void processInput();
  void update(float deltaTime);
  void render();

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
};
