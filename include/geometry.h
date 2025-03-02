#pragma once

#include "assets.h"
#include <string>

#include <toml++/toml.hpp>

class Geometry : public Asset {
public:
  // Compiles a vertex shader, implementation should guarantee success
  virtual void compile_vertex_shader(unsigned int &vert_shader) {};
  // Calls the corresponding draw functions
  virtual void draw_geometry() {};
  // Release resources allocated by Geometry
  virtual void destroy() {};

  virtual toml::table save() = 0;
};

// Geometry covering the entire viewport
class ScreenQuadGeometry : public Geometry {
private:
  inline static constexpr float VERTICES[12] = {
      -1.0f, -1.0f, 0.0f, //
      1.0f,  -1.0f, 0.0f, //
      -1.0f, 1.0f,  0.0f, //
      1.0f,  1.0f,  0.0f, //
  };
  inline static constexpr unsigned int INDICES[6] = {0, 1, 2, 2, 1, 3};
  inline static constexpr char *VERT_SRC = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)";

  unsigned int vao;
  unsigned int vbo;
  unsigned int ebo;

public:
  // Creates a new ScreenQuadGeometry
  ScreenQuadGeometry(std::string name = "FullscreenQuad");
  void compile_vertex_shader(unsigned int  &vert_shader) override;
  void draw_geometry() override;
  void destroy() override;

  toml::table save() override {
    return toml::table{{"type", "ScreenQuadGeometry"}, {"name", this->name}};
  }
};
