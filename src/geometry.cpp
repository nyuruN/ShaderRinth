#include "geometry.h"

#include <glad/gl.h>

//! Geometry

std::shared_ptr<Geometry> Geometry::load(toml::table &table, std::shared_ptr<AssetManager>) {
  std::string type = table["type"].value<std::string>().value();

  // We only have one geometry class for now...
  if (type == "ScreenQuadGeometry") {
    std::string name = table["name"].value<std::string>().value();
    return std::make_shared<ScreenQuadGeometry>(ScreenQuadGeometry(name));
  }

  throw std::runtime_error("Unknown geometry type!");
};

//! ScreenQuadGeometry

ScreenQuadGeometry::ScreenQuadGeometry(std::string name) {
  this->name = name;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
}
void ScreenQuadGeometry::compile_vertex_shader(GLuint &vert_shader) {
  vert_shader = glCreateShader(GL_VERTEX_SHADER);
  const char *src = VERT_SRC;
  glShaderSource(vert_shader, 1, &src, NULL);
  glCompileShader(vert_shader);
}
void ScreenQuadGeometry::draw_geometry() {
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}
void ScreenQuadGeometry::destroy() {
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
  glDeleteVertexArrays(1, &vao);
}
