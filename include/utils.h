#pragma once

#include <functional>
#include <imgui.h>
#include <memory>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <vector>

// TODO: Undo: Create a smart pointer that...
// Is used by both ImNodes and Self: Swap<T>.data()
// Keeps track of previous value: Swap<T>.prev()
// Is Generic: Swap<T>
// Commit to overwrite previous value: Swap<T>.commit()
// Overloads op= to overwrite both

// Forward declares
struct Action;
struct UndoContext;

// Helper struct for global data that need additional setup
struct Global {
public:
  UndoContext *undo_context;

  // Access a static instance of Global
  static Global &instance() {
    static Global instance;
    return instance;
  }
  // Initializes spdlog loggers
  void init();
  // Shuts down spdlog
  void shutdown() { spdlog::shutdown(); }
  // Create a logger with a custom name
  spdlog::logger create_logger(const char *name);
  // Gets the ringbuffer sink object for use in ConsoleWidget
  std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> get_ringbuffer_sink() {
    return ringbuffer_sink;
  }
  // Gets the current global undo context
  static inline UndoContext *getUndoContext() { return Global::instance().undo_context; }
  // Sets the current global undo context
  static inline void setUndoContext(UndoContext *ptr) { Global::instance().undo_context = ptr; }

private:
  Global() = default;
  ~Global() = default;
  Global(const Global &) = delete;
  Global &operator=(const Global &) = delete;

  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink;
  std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringbuffer_sink;
};

// TODO: Placeholder undo action
struct Action {
private:
  std::function<void()> do_func;
  std::function<void()> undo_func;

public:
  Action(
      std::function<void()> do_func = [] {}, std::function<void()> undo_func = [] {})
      : do_func(do_func), undo_func(undo_func) {}
  void Do() { do_func(); };
  void Undo() { undo_func(); };
};

struct UndoContext {
private:
  std::vector<Action> history = {};
  unsigned int idx = 0;
  unsigned int max_history = 50;
  unsigned int delete_interval = 10;

public:
  UndoContext(unsigned int max_history) { this->max_history = max_history; }
  // Executes given action and updates UndoContext
  void do_action(Action a);
  // Undo an action if available
  void undo();
  // Redo an action if available
  void redo();
  // Resets undo history, keeping last action
  void clear();
};

// Should be called once per frame
void updateKeyStates();
// Return true if the key transitioned from RELEASE to PRESS
bool isKeyJustPressed(ImGuiKey key);
