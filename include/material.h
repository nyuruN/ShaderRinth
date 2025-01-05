#include "config_app.h"
#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <cereal/cereal.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/string.hpp>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <spdlog/spdlog.h>

const std::filesystem::path DEFAULT_FRAG_PATH =
    std::filesystem::path(APP_ROOT) / "assets" / "shaders" /
    "default_frag.glsl";

class Geometry {
public:
  std::string name;

  virtual void compile_vertex_shader(unsigned int &vert_shader) {};
  virtual void draw_geometry() {};
  virtual void destroy_geometry() {};

  template <class Archive> void serialize(Archive &ar) { ar(name); }
};

struct Shader {
  char log[512] = {'\0'};
  std::filesystem::path path;
  std::string source;
  std::string name;
  GLuint program;
  bool compiled = false;

public:
  Shader(std::filesystem::path path, std::string name = "Default") {
    std::ifstream file(path);
    if (!file) {
      file = std::ifstream(DEFAULT_FRAG_PATH);
      if (!file)
        throw std::runtime_error("failed to open default_frag.glsl file");
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    source = buf.str();
    this->path = path;
    this->name = name;
  }
  template <class Archive>
  static void load_and_construct(Archive &ar,
                                 cereal::construct<Shader> &construct) {
    std::string name, path;
    ar(name, path);
    construct(std::filesystem::path(path), name);
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(name, path.string());
  }
  bool compile(std::shared_ptr<Geometry> geo) {
    spdlog::info("Compiling shaders");

    int success;
    const char *src = source.c_str();
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &src, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);

    if (!success) {
      spdlog::error("Failed to compile fragment shader");
      glGetShaderInfoLog(frag, 512, NULL, log);
      return success;
    }

    GLuint vert;
    geo->compile_vertex_shader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);

    if (!success) {
      spdlog::error("Failed to compile vertex shader");
      glGetShaderInfoLog(frag, 512, NULL, log);
      return success;
    }

    program = glCreateProgram();
    glAttachShader(program, frag);
    glAttachShader(program, vert);
    glLinkProgram(program);
    glGetProgramiv(program, GL_COMPILE_STATUS, &success);

    glDeleteShader(vert); // Detach shader and mark for deletion
    glDeleteShader(frag);

    if (!success) {
      spdlog::error("Failed to link program");
      glGetProgramInfoLog(program, 512, NULL, log);
      return success;
    }

    compiled = true;
    return success;
  }
  void use() { glUseProgram(program); }
  bool is_compiled() { return compiled; }
  void should_recompile() { compiled = false; }
  void set_source(std::string src) { source = src; }
  std::string &get_source() { return source; }
  void set_name(std::string name) { this->name = name; }
  std::string &get_name() { return name; }
  std::filesystem::path &get_path() { return path; }
  char *get_log() { return log; }
  GLuint get_uniform_loc(char *name) {
    return glGetUniformLocation(program, name);
  }
};

class ScreenQuadGeometry : public Geometry {
private:
  const float VERTICES[12] = {
      -1.0f, -1.0f, 0.0f, //
      1.0f,  -1.0f, 0.0f, //
      -1.0f, 1.0f,  0.0f, //
      1.0f,  1.0f,  0.0f, //
  };
  const unsigned int INDICES[6] = {0, 1, 2, 2, 1, 3};
  const char *VERT_SRC = R"(
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
  ScreenQuadGeometry(std::string name = "FullscreenQuad") {
    this->name = name;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
  }
  void compile_vertex_shader(GLuint &vert_shader) override {
    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader, 1, &VERT_SRC, NULL);
    glCompileShader(vert_shader);
  }
  void draw_geometry() override {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }
  void destroy_geometry() override {
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Geometry>(this));
  }
};

// Type registration
CEREAL_REGISTER_TYPE(ScreenQuadGeometry)
