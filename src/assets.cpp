#include "assets.h"

#include "shader.h"
#include "texture.h"

void AssetManager::destroy() {
  for (auto &pair : *textures) {
    pair.second->destroy();
  }
  for (auto &pair : *shaders) {
    pair.second->destroy();
  }
  geometries.reset();
  shaders.reset();
  textures.reset();
  graphs.reset();
}
