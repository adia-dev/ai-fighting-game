#pragma once

#include <SDL.h>
#include <memory>
#include <string>

struct SDLWindowDeleter {
  void operator()(SDL_Window *window) const {
    if (window)
      SDL_DestroyWindow(window);
  }
};

class Window {
public:
  Window(const std::string &title, int width, int height, Uint32 flags) {
    m_window.reset(SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED, width, height,
                                    flags));
    if (!m_window)
      throw std::runtime_error("SDL_CreateWindow Error: " +
                               std::string(SDL_GetError()));
  }
  SDL_Window *get() const { return m_window.get(); }

private:
  std::unique_ptr<SDL_Window, SDLWindowDeleter> m_window;
};
