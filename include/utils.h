#include <GLFW/glfw3.h>
#include <any>
#include <filesystem>
#include <imgui.h>
#include <map>
#include <material.h>
#include <string>

class AssetManager {
public:
  std::map<std::filesystem::path, std::shared_ptr<std::string>> sources;
  std::map<std::string, std::shared_ptr<Geometry>> geometries;
  std::map<std::string, std::shared_ptr<Shader>> shaders;

  void load_source(std::filesystem::path) {}
  // Throws std::out_of_range if the resource does not exist
  std::shared_ptr<Shader> get_shader(std::string &name) {
    return shaders.at(name);
  }
  std::shared_ptr<Geometry> get_geometry(std::string &name) {
    return geometries.at(name);
  }
  void set_geometry(std::string name, std::shared_ptr<Geometry> value) {
    geometries.insert(std::make_pair(name, value));
  }
  void set_shader(std::string name, std::shared_ptr<Shader> value) {
    shaders.insert(std::make_pair(name, value));
  }
};

static AssetManager assets;

void debugAny(std::any &value);
// Return true only if the key transitioned from RELEASE to PRESS
bool isKeyJustPressed(GLFWwindow *window, int key);
// Return true only if the key transitioned from RELEASE to PRESS
// (ImGui version)
bool isKeyJustPressed(ImGuiKey key);
