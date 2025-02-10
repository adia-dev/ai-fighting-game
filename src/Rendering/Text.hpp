#pragma once

#include "Resources/R.hpp"
#include <SDL_ttf.h>
#include <string>

inline static void drawText(SDL_Renderer *renderer, const std::string &text,
                            int x, int y, SDL_Color color, int fontSize = 14) {
  TTF_Font *font = TTF_OpenFont(R::font("seguiemj.ttf").c_str(), fontSize);
  if (!font)
    return;
  SDL_Surface *surface =
      TTF_RenderText_Blended_Wrapped(font, text.c_str(), color, 1000);
  if (!surface) {
    TTF_CloseFont(font);
    return;
  }
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_Rect dstRect = {x, y, surface->w, surface->h};
  SDL_FreeSurface(surface);
  SDL_RenderCopy(renderer, texture, NULL, &dstRect);
  SDL_DestroyTexture(texture);
  TTF_CloseFont(font);
}

inline static void drawCenteredText(SDL_Renderer *renderer,
                                    const std::string &text, int centerX,
                                    int centerY, SDL_Color color, float scale) {
  TTF_Font *font = TTF_OpenFont(R::font("seguiemj.ttf").c_str(),
                                static_cast<int>(24 * scale));
  if (!font)
    return;
  SDL_Surface *surface =
      TTF_RenderText_Blended_Wrapped(font, text.c_str(), color, 1000);
  if (!surface) {
    TTF_CloseFont(font);
    return;
  }
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  int w, h;
  SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
  SDL_Rect dst = {centerX - w / 2, centerY - h / 2, w, h};
  SDL_RenderCopy(renderer, texture, nullptr, &dst);
  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
  TTF_CloseFont(font);
}
