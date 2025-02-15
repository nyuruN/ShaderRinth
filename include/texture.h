#pragma once

#include <GLES3/gl3.h>
#include <filesystem>
#include <string>

#include <cereal/cereal.hpp> // for static load_and_construct
#include <cereal/types/string.hpp>
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
  template <class Archive>
  static void load_and_construct(Archive &ar, cereal::construct<Texture> &construct) {
    std::string path_str, name;
    ar(name, path_str);
    construct(name, path_str);
  }
  template <class Archive> void save(Archive &ar) const { ar(name, path.string()); }
  toml::table save();
};
