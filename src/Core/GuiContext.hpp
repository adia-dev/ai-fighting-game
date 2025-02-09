#pragma once
#include <SDL.h>
#include <imgui.h>
#include <string>

class GuiContext {
public:
  struct Config {
    ImGuiConfigFlags flags =
        ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    float fontScale = 1.0f;
    bool customMouseCursor = false;
    std::string iniFilename = "imgui.ini";
  };

  GuiContext() = default;
  ~GuiContext();

  void init(SDL_Window *window, SDL_Renderer *renderer, Config &config);
  void cleanup();

  void beginFrame();
  void endFrame();

  bool processEvent(const SDL_Event &event);

  // Game viewport management
  void createGameViewport(int width, int height);
  SDL_Texture *getGameViewportTexture();
  void beginGameViewportRender();
  void endGameViewportRender();
  void setupDefaultLayout();

  // DPI Scale access
  float getDpiScale() const { return m_dpiScale; }

private:
  void setupStyle();
  void updateDpiScale();

  SDL_Window *m_window = nullptr;
  SDL_Renderer *m_renderer = nullptr;
  bool m_initialized = false;
  Config m_config;
  float m_dpiScale = 1.0f;

  // Game viewport texture
  SDL_Texture *m_gameViewportTexture = nullptr;
  int m_gameViewportWidth = 0;
  int m_gameViewportHeight = 0;
};
