
#include "Animation.hpp"
#include "Animator.hpp"
#include "Character.hpp"
#include "Mover.hpp"
#include "PiksyAnimationLoader.hpp"
#include "Vector2f.hpp"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

bool checkCollision(const SDL_Rect &a, const SDL_Rect &b);
void resolveCollision(Character &a, Character &b);
void applyCollisionImpulse(Character &a, Character &b);

int main(int argc, char *argv[]) {

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
    return 1;
  }
  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    std::cerr << "IMG_Init Error: " << IMG_GetError() << "\n";
    SDL_Quit();
    return 1;
  }
  if (TTF_Init() < 0) {
    std::cerr << "TTF_Init Error: " << TTF_GetError() << "\n";
    IMG_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow(
      "Controllable Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 1;
  }
  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_Texture *spritesheet = IMG_LoadTexture(
      renderer,
      "/Users/adiadev/Projects/Dev/C++/piksy/resources/textures/alex.png");
  if (!spritesheet) {
    std::cerr << "IMG_LoadTexture Error: " << IMG_GetError() << "\n";
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 1;
  }

  Animator animatorPlayer(spritesheet);
  Animator animatorEnemy(spritesheet);

  Animation walkAnimation;
  try {
    walkAnimation = PiksyAnimationLoader::loadAnimation(
        "/Users/adiadev/Projects/Dev/C++/piksy/exports/Walk.json");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load animation: " << e.what() << "\n";

    Frame frame;
    frame.frameRect = {0, 2203, 116, 120};
    frame.flipped = false;
    frame.duration_ms = 100;
    walkAnimation.name = "Walk";
    walkAnimation.loop = true;
    walkAnimation.frames.push_back(frame);
  }
  animatorPlayer.setAnimation(walkAnimation);
  animatorEnemy.setAnimation(walkAnimation);

  Character player(&animatorPlayer);
  Character enemy(&animatorEnemy);

  player.mover.position = Vector2f(100, 100);
  enemy.mover.position = Vector2f(400, 100);

  bool quit = false;
  Uint32 lastTime = SDL_GetTicks();

  while (!quit) {

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        quit = true;
    }

    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.f;
    lastTime = currentTime;

    const Uint8 *keystate = SDL_GetKeyboardState(NULL);
    player.handleInput(keystate);

    Vector2f toPlayer = player.mover.position - enemy.mover.position;
    if (toPlayer.length() > 0.0f) {
      Vector2f force = toPlayer.normalized() * 200.0f;
      enemy.mover.applyForce(force);
    }

    player.update(deltaTime);
    enemy.update(deltaTime);

    auto clampPosition = [](Character &ch) {
      SDL_Rect r = ch.animator->getCurrentFrameRect();

      if (ch.mover.position.x < 0)
        ch.mover.position.x = 0;
      if (ch.mover.position.x > WINDOW_WIDTH - r.w)
        ch.mover.position.x = WINDOW_WIDTH - r.w;

      if (ch.mover.position.y < 0)
        ch.mover.position.y = 0;
    };
    clampPosition(player);
    clampPosition(enemy);

    if (checkCollision(player.getCollisionRect(), enemy.getCollisionRect())) {
      resolveCollision(player, enemy);
      applyCollisionImpulse(player, enemy);

      player.applyDamage(1);
      enemy.applyDamage(1);
    }

    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
    SDL_RenderDrawLine(renderer, 0, GROUND_LEVEL, WINDOW_WIDTH, GROUND_LEVEL);

    player.render(renderer);
    enemy.render(renderer);

    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  SDL_DestroyTexture(spritesheet);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();

  return 0;
}

bool checkCollision(const SDL_Rect &a, const SDL_Rect &b) {
  return SDL_HasIntersection(&a, &b);
}

void resolveCollision(Character &a, Character &b) {
  SDL_Rect rectA = a.getCollisionRect();
  SDL_Rect rectB = b.getCollisionRect();
  if (!checkCollision(rectA, rectB))
    return;

  SDL_Rect intersection;
  SDL_IntersectRect(&rectA, &rectB, &intersection);

  if (intersection.w < intersection.h) {
    int separation = intersection.w / 2;
    if (rectA.x < rectB.x) {
      a.mover.position.x -= separation;
      b.mover.position.x += separation;
    } else {
      a.mover.position.x += separation;
      b.mover.position.x -= separation;
    }
  } else {
    int separation = intersection.h / 2;
    if (rectA.y < rectB.y) {
      a.mover.position.y -= separation;
      b.mover.position.y += separation;
    } else {
      a.mover.position.y += separation;
      b.mover.position.y -= separation;
    }
  }
}

void applyCollisionImpulse(Character &a, Character &b) {
  SDL_Rect rectA = a.getCollisionRect();
  SDL_Rect rectB = b.getCollisionRect();

  Vector2f centerA(rectA.x + rectA.w / 2.0f, rectA.y + rectA.h / 2.0f);
  Vector2f centerB(rectB.x + rectB.w / 2.0f, rectB.y + rectB.h / 2.0f);

  Vector2f collisionNormal = (centerB - centerA).normalized();
  float impulseStrength = 500.0f;

  a.mover.applyForce(collisionNormal * -impulseStrength);
  b.mover.applyForce(collisionNormal * impulseStrength);
}
