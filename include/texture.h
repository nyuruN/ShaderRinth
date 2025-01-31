#pragma once

#include <GLES3/gl3.h>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <filesystem>
#include <stb_image.h>
#include <stb_image_write.h>

struct Texture {
private:
  std::string name;
  std::filesystem::path path;
  int width = 0, height = 0;
  int channels = 0;

  bool loaded = false;

  GLuint texture = 0;

public:
  /*
  ~Texture() {
    if (texture != 0)
      glDeleteTextures(1, &texture);
  }
  // No copy
  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;
  // Enable moving
  Texture(Texture &&other) noexcept
      : texture(other.texture), path(std::move(other.path)), width(other.width),
        height(other.height), channels(other.channels) {
    other.texture = 0; // Prevent double deletion
  }
  Texture &operator=(Texture &&other) noexcept {
    if (this != &other) { // Avoid self assign
      if (texture != 0) { // Delete current
        glDeleteTextures(1, &texture);
      }
      texture = other.texture;
      path = std::move(other.path);
      loaded = other.loaded;
      other.texture = 0; // Prevent double deletion
    }
    return *this;
  }
  */

  operator bool() const { return loaded; }
  Texture(std::string name, std::filesystem::path path)
      : name(name), path(path) {
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data =
        stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (data == NULL)
      return;

    // Placeholder format checking
    int format = (channels <= 3) ? GL_RGB : GL_RGBA;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound
    // texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,
                 0,       // mip map
                 GL_RGBA, // format to store
                 width, height,
                 0, // always 0
                 format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    loaded = true;
  }
  template <class Archive>
  static void load_and_construct(Archive &ar,
                                 cereal::construct<Texture> &construct) {
    std::string path_str, name;
    ar(name, path_str);
    construct(name, std::filesystem::path(path_str));
  }
  template <class Archive> void save(Archive &ar) const {
    ar(name, path.string());
  }

  GLuint get_texture() { return texture; }
  std::string &get_name() { return name; }
  void destroy() {
    if (texture != 0)
      glDeleteTextures(1, &texture);
  }
};
