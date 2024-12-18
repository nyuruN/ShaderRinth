#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <spdlog/spdlog.h>

// Reserved for later use
class Geometry {

public:
  virtual void compile_vertex_shader(unsigned int &vert_shader) {};
  virtual void draw_geometry() {};
  virtual void destroy_geometry() {};
};

class Shader {
  unsigned int shader;
  unsigned int program;
  int success;
  char *source;
  bool compiled = false;

public:
  Shader(std::filesystem::path path) {
    std::ifstream file(path);
    if (!file)
      throw std::runtime_error("Failed to open file: " + path.string());

    std::ostringstream buf;
    buf << file.rdbuf();
    source = buf.str().data();
  }
  Shader(char *src) { source = src; }
  bool compile(std::shared_ptr<Geometry> geo) {
    shader = glCreateShader(GL_FRAGMENT_SHADER);

    spdlog::info("Embedding Sources");
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    spdlog::info("Compiled fragment");

    if (!success) {
      spdlog::error("Failed to compile Shader");
      return success;
    }

    spdlog::info("Compiling vertex");
    unsigned int vert;
    geo->compile_vertex_shader(vert);

    spdlog::info("Linking Program");
    program = glCreateProgram();
    glAttachShader(program, shader);
    glAttachShader(program, vert);
    glLinkProgram(program);
    glGetProgramiv(program, GL_COMPILE_STATUS, &success);

    if (!success) {
      spdlog::error("Failed to link program");
      return success;
    }

    spdlog::info("Compiling fragment shader");
    compiled = true;

    return success;
  }
  void use() { glUseProgram(program); }
  bool is_compiled() { return compiled; }
  char *get_log() {
    if (!success) {
      char buf[512] = {'\0'};
      glGetShaderInfoLog(shader, 512, NULL, buf);
      if (buf[0] != '\0')
        return buf;
      glGetProgramInfoLog(program, 512, NULL, buf);
      return buf;
    }
  }
  unsigned int get_uniform_loc(char *name) {
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

  unsigned int vao;
  unsigned int vbo;
  unsigned int ebo;

public:
  ScreenQuadGeometry() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    spdlog::info("Generating buffers");
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
  void compile_vertex_shader(unsigned int &vert_shader) override {
    spdlog::info("Compiling Vertex Shader");
    int success;
    char info_log[512];

    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader, 1, &VERT_SRC, NULL);
    glCompileShader(vert_shader);

    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
      spdlog::error("VERTEX Shader compilation failed!");
      glGetShaderInfoLog(vert_shader, 512, NULL, info_log);
      spdlog::error(info_log);
    }

    spdlog::info("Finished compiling vertex shader");
  }
  void draw_geometry() override {
    spdlog::info("Drawing geometry");
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }
  void destroy_geometry() override {
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
  }
};
