#pragma once

#include "assets.h"

#include <filesystem>
#include <glad/glad.h>
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

  toml::table save(std::filesystem::path) override {
    return toml::table{
        {"name", name},         //
        {"path", path.string()} //
    };
  };
  static std::shared_ptr<Texture> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    std::string name = tbl["name"].value<std::string>().value();
    std::string path_str = tbl["path"].value<std::string>().value(); // Absolute path

    return std::make_shared<Texture>(Texture(name, path_str));
  }
};
