#pragma once
#include <SDL.h>
#include <memory>
#include <string>

struct SDLRendererDeleter {
  void operator()(SDL_Renderer *renderer) const {
    if (renderer)
      SDL_DestroyRenderer(renderer);
  }
};

class Renderer {
public:
  Renderer(SDL_Window *window, Uint32 flags) {
    m_renderer.reset(SDL_CreateRenderer(window, -1, flags));
    if (!m_renderer)
      throw std::runtime_error("SDL_CreateRenderer Error: " +
                               std::string(SDL_GetError()));
  }
  SDL_Renderer *get() const { return m_renderer.get(); }

private:
  std::unique_ptr<SDL_Renderer, SDLRendererDeleter> m_renderer;
};
