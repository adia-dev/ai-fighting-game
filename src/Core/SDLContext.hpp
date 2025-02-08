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
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      Logger::error("SDL_Init Error: " + std::string(SDL_GetError()));
      throw std::runtime_error("SDL_Init Error: " +
                               std::string(SDL_GetError()));
    }
    Logger::debug("SDL initialized successfully.");

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
