#pragma once
#include "Texture2D.hpp"
#include <memory>
#include <string>
#include <unordered_map>

class ResourceManager {
public:
  ResourceManager(SDL_Renderer *renderer) : m_renderer(renderer) {}

  std::shared_ptr<Texture2D> getTexture(const std::string &filePath) {
    if (m_textures.find(filePath) != m_textures.end())
      return m_textures[filePath];
    auto texture = std::make_shared<Texture2D>(m_renderer, filePath);
    m_textures[filePath] = texture;
    return texture;
  }

private:
  SDL_Renderer *m_renderer;
  std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textures;
};
