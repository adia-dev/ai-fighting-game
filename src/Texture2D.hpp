// Texture2D.hpp
#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <memory>
#include <stdexcept>
#include <string>

struct SDLTextureDeleter {
  void operator()(SDL_Texture *texture) const {
    if (texture)
      SDL_DestroyTexture(texture);
  }
};

class Texture2D {
public:
  Texture2D(SDL_Renderer *renderer, const std::string &filePath) {
    m_texture.reset(IMG_LoadTexture(renderer, filePath.c_str()));
    if (!m_texture) {
      throw std::runtime_error("IMG_LoadTexture Error: " +
                               std::string(IMG_GetError()));
    }
    SDL_QueryTexture(m_texture.get(), nullptr, nullptr, &m_width, &m_height);
  }
  SDL_Texture *get() const { return m_texture.get(); }
  int width() const { return m_width; }
  int height() const { return m_height; }

private:
  std::unique_ptr<SDL_Texture, SDLTextureDeleter> m_texture;
  int m_width, m_height;
};
