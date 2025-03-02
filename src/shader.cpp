#include "shader.h"
#include "data.h"
#include "geometry.h"

#include <fstream>
#include <glad/glad.h>

// Uniform setters
static const std::function<void(GLint, Data)> SET_UNIFORM[] = {
    [](GLint l, Data v) { glUniform1i(l, v.get<Data::Int>()); },
    [](GLint l, Data v) { glUniform2iv(l, 1, v.get<Data::IVec2>().data()); },
    [](GLint l, Data v) { glUniform3iv(l, 1, v.get<Data::IVec3>().data()); },
    [](GLint l, Data v) { glUniform4iv(l, 1, v.get<Data::IVec4>().data()); },
    [](GLint l, Data v) { glUniform1f(l, v.get<Data::Float>()); },
    [](GLint l, Data v) { glUniform2fv(l, 1, v.get<Data::Vec2>().data()); },
    [](GLint l, Data v) { glUniform3fv(l, 1, v.get<Data::Vec3>().data()); },
    [](GLint l, Data v) { glUniform4fv(l, 1, v.get<Data::Vec4>().data()); },
    [](GLint l, Data v) { /* Data::Texture2D */ },
};

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
Shader::Shader(std::string name, std::filesystem::path project_root,
               std::filesystem::path rel_path) {
  auto abs_path = project_root / rel_path;
  std::ifstream file(abs_path);
  if (!file) {
    spdlog::error("Failed to load shader \"{}\" in \"{}\"!", name, abs_path.string());
    return;
  }
  std::ostringstream buf;
  buf << file.rdbuf();
  source = buf.str();
  this->name = name;
  this->path = rel_path;
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
void Shader::set_uniform(const char *name, Data data) {
  GLuint loc = get_uniform_loc(name);

  if (data.type == DataType::Texture2D) {
    GLuint texture = data.get<Data::Texture2D>();

    // Gets next texture unit
    int offset = bound_textures.size();
    bound_textures.push_back(texture);
    GLenum unit = GL_TEXTURE0 + unit;

    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(loc, bound_textures.size() - 1);
  } else
    SET_UNIFORM[data.type](loc, data);
}
toml::table Shader::save(std::filesystem::path project_root) {
  auto abs_path = project_root / path;
  // Ensure the parent directory exists
  std::filesystem::create_directories(abs_path.parent_path());

  if (auto file = std::ofstream(abs_path))
    file << source;
  else
    spdlog::error("Failed to save shader \"{}\" in \"{}\"!", name, abs_path.string());

  return toml::table{
      {"name", name},
      {"path", path.string()},
  };
}
