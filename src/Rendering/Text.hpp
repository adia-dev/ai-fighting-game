#include "Resources/R.hpp"
#include <SDL_ttf.h>
#include <string>

inline static void drawText(SDL_Renderer *renderer, const std::string &text,
                            int x, int y, SDL_Color color) {
  TTF_Font *font = TTF_OpenFont(R::font("seguiemj.ttf").c_str(), 14);
  if (!font)
    return;
  SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), color);
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
