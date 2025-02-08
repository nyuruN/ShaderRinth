#pragma once

#include "config_app.h"
#include <GLES3/gl3.h>
#include <filesystem>
#include <string>

#include <cereal/cereal.hpp>
#include <fstream>
#include <spdlog/spdlog.h>
#include <utils.h>

static const std::filesystem::path DEFAULT_FRAG_PATH =
    std::filesystem::path(APP_ROOT) / "assets" / "shaders" /
    "default_frag.glsl";

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
  // Creates a new fragment shader, does not load yet
  Shader(std::string name, std::filesystem::path path, std::string source) {
    this->name = name;
    this->path = path;
    this->source = source;
  }
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
  GLuint get_uniform_loc(const char *name) {
    return glGetUniformLocation(program, name);
  }
  bool is_compiled() { return compiled; }
  std::string &get_source() { return source; }
  std::string &get_name() { return name; }
  std::filesystem::path get_path() { return path; }
  char *get_log() { return log; }
  template <class Archive>
  static void load_and_construct(Archive &ar,
                                 cereal::construct<Shader> &construct) {
    std::string name, path, source;
    ar(name, path);

    std::filesystem::path abs_path = Global::instance().project_root / path;
    std::ifstream file(abs_path);
    if (!file) {
      spdlog::error("failed to load shader \"{}\" in \"{}\"!", name,
                    abs_path.string());
      return;
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    source = buf.str();

    construct(name, path, source);
  }
  template <class archive> void save(archive &ar) const {
    auto abs_path = Global::instance().project_root / path;
    // Ensure the parent directory exists
    std::filesystem::create_directories(abs_path.parent_path());

    std::ofstream file(abs_path);
    if (!file) {
      spdlog::error("failed to save shader \"{}\" in \"{}\"!", name,
                    abs_path.string());
      return;
    }
    file << source;

    ar(VP(name), NVP("path", path.string()));
  }
};
