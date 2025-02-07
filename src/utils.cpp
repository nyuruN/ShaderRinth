#include "utils.h"

//! Global

void Global::init() {
  console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::debug);
  ringbuffer_sink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(512);
  ringbuffer_sink->set_level(spdlog::level::info);
  ringbuffer_sink->set_pattern("[%n] [%l] %v");

  auto logger = create_logger("Global");
  spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
}
spdlog::logger Global::create_logger(char *name) {
  std::vector<spdlog::sink_ptr> sinks = {ringbuffer_sink, console_sink};
  return spdlog::logger(name, sinks.begin(), sinks.end());
}

//! UndoContext

void UndoContext::do_action(Action a) {
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
void UndoContext::undo() {
  if (history.empty() || idx == 0)
    return;
  spdlog::info("[Undo] History size: {}, Index: {}--", history.size(), idx);
  history.at(idx).Undo();
  idx--;
};
void UndoContext::redo() {
  if (history.empty() || history.size() - 1 <= idx)
    return;
  spdlog::info("[Redo] History size: {}, Index: {}++", history.size(), idx);
  idx++;
  history.at(idx).Do();
};
void UndoContext::clear() {
  if (history.empty())
    return;
  Action a = std::move(history.at(idx));
  history.clear();
  history.push_back(a);
};

//! Input

static std::map<ImGuiKey, bool> previousStates;
static std::map<ImGuiKey, bool> currentStates;

void updateKeyStates() { previousStates.swap(currentStates); }
bool isKeyJustPressed(ImGuiKey key) {
  bool current = ImGui::IsKeyDown(key);
  currentStates[key] = current;
  return !previousStates[key] && current;
}
