#pragma once

#include "assets.h"

#include <glad/glad.h>
#include <filesystem>
#include <string>

#include <toml++/toml.h>

class Texture : public Asset {
private:
  std::filesystem::path path;
  int width = 0, height = 0;
  int channels = 0;

  bool loaded = false;
  GLuint texture = 0;

public:
  operator bool() const { return loaded; }

  Texture(std::string name, std::filesystem::path path);
  void destroy() {
    if (texture != 0)
      glDeleteTextures(1, &texture);
  }
  GLuint get_texture() { return texture; }
  bool is_loaded() { return loaded; }
  toml::table save() {
    return toml::table{
        {"name", name},
        {"path", path.string()},
    };
  };
};
