#include "Animator.hpp"
#include "PiksyAnimationLoader.hpp"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

int main(int argc, char *argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow(
      "Piksy Animation MVP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    std::cerr << "IMG_Init Error: " << IMG_GetError() << "\n";
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_Texture *spritesheet = IMG_LoadTexture(
      renderer,
      "/Users/adiadev/Projects/Dev/C++/piksy/resources/textures/alex.png");
  if (!spritesheet) {
    std::cerr << "IMG_LoadTexture Error: " << IMG_GetError() << "\n";
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  Animation animation;
  try {
    animation = PiksyAnimationLoader::loadAnimation(
        "/Users/adiadev/Projects/Dev/C++/piksy/exports/Walk.json");
  } catch (const std::exception &e) {
    std::cerr << "Error loading animation: " << e.what() << "\n";
    SDL_DestroyTexture(spritesheet);
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  Animator animator(spritesheet);
  animator.setAnimation(animation);

  bool quit = false;
  SDL_Event event;
  Uint32 lastTime = SDL_GetTicks();

  while (!quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        quit = true;
    }

    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = static_cast<float>(currentTime - lastTime);
    lastTime = currentTime;

    animator.update(deltaTime);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    animator.render(renderer, 100, 100);

    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  SDL_DestroyTexture(spritesheet);
  IMG_Quit();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
