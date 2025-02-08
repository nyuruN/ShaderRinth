#pragma once

#include <GLES3/gl3.h>
#include <string>

class Geometry {
protected:
  std::string name;

public:
  // Compiles a vertex shader, implementation should guarantee success
  virtual void compile_vertex_shader(unsigned int &vert_shader) {};
  // Calls the corresponding draw functions
  virtual void draw_geometry() {};
  // Release resources allocated by Geometry
  virtual void destroy() {};
  std::string &get_name() { return name; }

  template <class Archive> void serialize(Archive &ar) { ar(name); }
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

  GLuint vao;
  GLuint vbo;
  GLuint ebo;

public:
  // Creates a new ScreenQuadGeometry
  ScreenQuadGeometry(std::string name = "FullscreenQuad");
  void compile_vertex_shader(GLuint &vert_shader) override;
  void draw_geometry() override;
  void destroy() override;
  template <class Archive> void serialize(Archive &ar);
};
