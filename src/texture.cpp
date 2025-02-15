//! Define only once in source file!

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#endif

#include "texture.h"

//! Texture

Texture::Texture(std::string name, std::filesystem::path path) : name(name), path(path) {
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);

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
#include <toml++/toml.h>
toml::table Texture::save() {
  return toml::table{
      {"name", name},
      {"path", path.string()},
  };
}
