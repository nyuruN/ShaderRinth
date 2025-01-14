#pragma once

#include <GLFW/glfw3.h>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#define VP(T) ::cereal::make_nvp(#T, T)
#define NVP(S, T) ::cereal::make_nvp(S, T)

template <typename T> using Assets = std::map<std::string, std::shared_ptr<T>>;

struct Global {
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

// TODO: Placeholder undo action
struct Action {
private:
  std::function<void()> do_func;
  std::function<void()> undo_func;

public:
  Action(
      std::function<void()> do_func = [] {},
      std::function<void()> undo_func = [] {})
      : do_func(do_func), undo_func(undo_func) {}
  void Do() { do_func(); };
  void Undo() { undo_func(); };
};

// TODO: Placeholder undo history
struct UndoHistory {
private:
  std::vector<Action> history = {};
  uint idx = 0;
  uint max_history = 50;
  uint delete_interval = 10;

public:
  void do_action(Action a) {
    if (history.size() - 1 > idx) // Truncate redos
      history.resize(idx + 1);
    if (history.size() > max_history + delete_interval) {
      // Delete history in bulk
      history.erase(history.begin(), history.begin() + delete_interval);
      idx -= delete_interval;
    }
    a.Do();
    history.push_back(a);
    idx += 1;
  };
  void undo() {
    if (history.empty())
      return;
    history.at(idx).Undo();
    idx -= 1;
  };
  void redo() {
    if (history.size() - 1 == idx)
      return;
    idx += 1;
    history.at(idx).Do();
  };
  void clear() {
    Action a = std::move(history.at(idx));
    history.clear();
    history.push_back(a);
  };
};

// Return true only if the key transitioned from RELEASE to PRESS
bool isKeyJustPressed(GLFWwindow *window, int key);
// Return true only if the key transitioned from RELEASE to PRESS
// (ImGui version)
bool isKeyJustPressed(ImGuiKey key);
