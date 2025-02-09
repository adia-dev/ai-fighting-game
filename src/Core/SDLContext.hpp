// src/Core/SDLContext.hpp
#pragma once
#include "Core/Logger.hpp"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdexcept>
#include <string>

class SDLContext {
public:
  SDLContext() {
#ifdef __EMSCRIPTEN__
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
        0) {
      Logger::error("SDL_Init Error: " + std::string(SDL_GetError()));
      throw std::runtime_error("SDL_Init Error: " +
                               std::string(SDL_GetError()));
    }
    Logger::debug("SDL initialized successfully.");

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
      SDL_Quit();
      Logger::error("IMG_Init Error: " + std::string(IMG_GetError()));
      throw std::runtime_error("IMG_Init Error: " +
                               std::string(IMG_GetError()));
    }
    Logger::debug("SDL_image initialized successfully.");

    if (TTF_Init() < 0) {
      IMG_Quit();
      SDL_Quit();
      Logger::error("TTF_Init Error: " + std::string(TTF_GetError()));
      throw std::runtime_error("TTF_Init Error: " +
                               std::string(TTF_GetError()));
    }
    Logger::debug("SDL_ttf initialized successfully.");
  }
  ~SDLContext() {
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    Logger::debug("SDL subsystems shut down.");
  }
};
