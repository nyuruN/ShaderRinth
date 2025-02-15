#pragma once

#include <GLES3/gl3.h>
#include <filesystem>
#include <string>

// #include <toml++/toml.hpp>

namespace toml {
inline namespace v3 {
class table;
}
} // namespace toml

struct Texture {
private:
  std::string name;
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
  std::string &get_name() { return name; }
  toml::table save();
};
