#include "shader.h"
#include "geometry.h"

//! Shader

Shader::Shader(std::string name) {
  std::ifstream file(DEFAULT_FRAG_PATH);
  if (!file)
    throw std::runtime_error("failed to open default_frag.glsl");
  std::ostringstream buf;
  buf << file.rdbuf();

  std::string n(name);
  path = std::filesystem::path("shaders") / n.append(".glsl");
  source = buf.str();
  this->name = name;
}
bool Shader::compile(std::shared_ptr<Geometry> geo) {
  int success;
  const char *src = source.c_str();
  GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag, 1, &src, NULL);
  glCompileShader(frag);
  glGetShaderiv(frag, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(frag, 512, NULL, log);
    return success;
  }

  GLuint vert;
  geo->compile_vertex_shader(vert);
  glGetShaderiv(vert, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(frag, 512, NULL, log);
    return success;
  }

  if (program != 0)
    glDeleteProgram(program);
  program = glCreateProgram();
  glAttachShader(program, frag);
  glAttachShader(program, vert);
  glLinkProgram(program);
  glGetProgramiv(program, GL_COMPILE_STATUS, &success);

  glDeleteShader(vert); // Detach shader and mark for deletion
  glDeleteShader(frag);

  if (!success) {
    glGetProgramInfoLog(program, 512, NULL, log);
    return success;
  }

  compiled = true;
  return success;
}
