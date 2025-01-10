#pragma once

#include <GLFW/glfw3.h>
#include <filesystem>
#include <imgui.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#define VP(T) ::cereal::make_nvp(#T, T)
#define NVP(S, T) ::cereal::make_nvp(S, T)

template <typename T> using Assets = std::map<std::string, std::shared_ptr<T>>;

class Global {
public:
  static Global &instance() {
    static Global instance;
    return instance;
  }

  void set_project_root(std::filesystem::path path) {
    std::lock_guard<std::mutex> lock(_mutex);
    project_root = path;
  }
  std::filesystem::path get_project_root() {
    std::lock_guard<std::mutex> lock(_mutex);
    return project_root;
  }

private:
  Global() = default;
  ~Global() = default;
  Global(const Global &) = delete;
  Global &operator=(const Global &) = delete;

  std::mutex _mutex;
  std::filesystem::path project_root;
};

// Return true only if the key transitioned from RELEASE to PRESS
bool isKeyJustPressed(GLFWwindow *window, int key);
// Return true only if the key transitioned from RELEASE to PRESS
// (ImGui version)
bool isKeyJustPressed(ImGuiKey key);
