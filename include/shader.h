#pragma once

#include "config_app.h"
#include <GLES3/gl3.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <string>

#include <toml++/toml.hpp>

static const std::filesystem::path DEFAULT_FRAG_PATH =
    std::filesystem::path(APP_ROOT) / "assets" / "shaders" / "default_frag.glsl";

// Forward declares
class Geometry;
struct Data;

struct Shader {
private:
  char log[512] = {'\0'};
  std::filesystem::path path; //  is RELATIVE to project_root
  std::string source;
  std::string name;

  std::vector<GLuint> bound_textures = {};
  GLuint program = 0;
  bool compiled = false;

public:
  operator bool() const { return compiled; }

  // Creates a new default fragment shader
  Shader(std::string name);
  // Creates a new fragment shader and loads its source
  Shader(std::string name, std::filesystem::path project_root, std::filesystem::path path);
  // Compiles the shader given a Geometry (mesh, vertex shader)
  bool compile(std::shared_ptr<Geometry> geo);
  // Destroys created program
  void destroy() { glDeleteProgram(program); }
  // Use the shader for render
  void use() { glUseProgram(program); }
  // Marks the shader for recompilation
  void recompile() { compiled = false; }
  // Marks the shader for recompilation
  void recompile_with_source(std::string src) {
    source = src;
    compiled = false;
  }
  // Sets shader uniforms
  void set_uniform(const char *name, Data data);
  // Clears bound textures
  void clear_textures() { bound_textures.clear(); }
  // If one needs to manually set uniforms
  GLuint get_uniform_loc(const char *name) { return glGetUniformLocation(program, name); }
  bool is_compiled() { return compiled; }
  std::string &get_source() { return source; }
  std::string &get_name() { return name; }
  std::filesystem::path get_path() { return path; }
  char *get_log() { return log; }

  toml::table save(std::filesystem::path project_root);
};
