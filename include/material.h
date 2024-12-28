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
  GLuint program;
  std::string source;
  bool compiled = false;
  char log[512] = {'\0'};

public:
  Shader(std::string src) { source = src; }
  Shader(std::filesystem::path path) {
    std::ifstream file(path);
    if (!file)
      throw std::runtime_error("Failed to open file: " + path.string());

    std::ostringstream buf;
    buf << file.rdbuf();
    source = buf.str().data();
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
  char *get_log() { return log; }
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

  GLuint vao;
  GLuint vbo;
  GLuint ebo;

public:
  ScreenQuadGeometry() {
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
};
