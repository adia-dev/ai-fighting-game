
#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdexcept>
#include <string>

class SDLContext {
public:
  SDLContext() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      throw std::runtime_error("SDL_Init Error: " +
                               std::string(SDL_GetError()));
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
      SDL_Quit();
      throw std::runtime_error("IMG_Init Error: " +
                               std::string(IMG_GetError()));
    }
    if (TTF_Init() < 0) {
      IMG_Quit();
      SDL_Quit();
      throw std::runtime_error("TTF_Init Error: " +
                               std::string(TTF_GetError()));
    }
  }
  ~SDLContext() {
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
  }
};
