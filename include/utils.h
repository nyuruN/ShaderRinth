#pragma once

#include <GLFW/glfw3.h>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <map>
#include <memory>
#include <mutex>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#define VP(T) ::cereal::make_nvp(#T, T)
#define NVP(S, T) ::cereal::make_nvp(S, T)

template <typename T> using Assets = std::map<std::string, std::shared_ptr<T>>;

// TODO: Create a smart pointer that...
// Is used by both ImNodes and Self: Swap<T>.data()
// Keeps track of previous value: Swap<T>.prev()
// Is Generic: Swap<T>
// Commit to overwrite previous value: Swap<T>.commit()
// Overloads op= to overwrite both

struct Action;
struct UndoContext;

struct Global {
public:
  static Global &instance() {
    static Global instance;
    return instance;
  }
  void init() {
    log_stream = std::make_shared<std::ostringstream>();
    console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    ostream_sink =
        std::make_shared<spdlog::sinks::ostream_sink_mt>(*log_stream);
    ostream_sink->set_level(spdlog::level::info);

    auto logger = create_logger("Global");
    spdlog::set_default_logger(logger);
  }
  std::string get_log_stream_string() {
    std::lock_guard<std::mutex> lock(_mutex);
    return log_stream->str();
  }
  std::shared_ptr<spdlog::logger> create_logger(char *name) {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<spdlog::sink_ptr> sinks = {ostream_sink, console_sink};
    auto logger =
        std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    return logger;
  }
  // Sets the current project directory for use in serialization
  void set_project_root(std::filesystem::path path) {
    std::lock_guard<std::mutex> lock(_mutex);
    project_root = path;
  }
  std::filesystem::path get_project_root() {
    std::lock_guard<std::mutex> lock(_mutex);
    return project_root;
  }
  void set_undo_context(UndoContext *context) { undo_context = context; }
  UndoContext *get_undo_context() { return undo_context; };

private:
  Global() = default;
  ~Global() = default;
  Global(const Global &) = delete;
  Global &operator=(const Global &) = delete;

  std::shared_ptr<std::ostringstream> log_stream;
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink;
  std::shared_ptr<spdlog::sinks::ostream_sink_mt> ostream_sink;
  std::mutex _mutex;
  std::filesystem::path project_root;
  UndoContext *undo_context;
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

struct UndoContext {
private:
  std::vector<Action> history = {};
  uint idx = 0;
  uint max_history = 50;
  uint delete_interval = 10;

public:
  UndoContext(uint max_history) { this->max_history = max_history; }
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
    idx++;
  };
  void undo() {
    if (history.empty() || idx == 0)
      return;
    spdlog::info("[Undo] History size: {}, Index: {}--", history.size(), idx);
    history.at(idx).Undo();
    idx--;
  };
  void redo() {
    if (history.empty() || history.size() - 1 <= idx)
      return;
    spdlog::info("[Redo] History size: {}, Index: {}++", history.size(), idx);
    idx++;
    history.at(idx).Do();
  };
  void clear() {
    if (history.empty())
      return;
    Action a = std::move(history.at(idx));
    history.clear();
    history.push_back(a);
  };
};

inline UndoContext *getUndoContext() {
  return Global::instance().get_undo_context();
}
inline void setUndoContext(UndoContext *ptr) {
  Global::instance().set_undo_context(ptr);
}

// Should be called per frame
void updateKeyStates();
// Return true only if the key transitioned from RELEASE to PRESS
bool isKeyJustPressed(ImGuiKey key);
