#pragma once
#include <SDL.h>

class Input {
public:
  static bool isKeyDown(SDL_Scancode key) {
    static const Uint8 *state = SDL_GetKeyboardState(NULL);
    return state[key] != 0;
  }
};
