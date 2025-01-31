#include <GLFW/glfw3.h>
#include <imgui.h>
#include <map>
#include <spdlog/spdlog.h>

static std::map<ImGuiKey, bool> previousStates;
static std::map<ImGuiKey, bool> currentStates;

// Should be called per frame
void updateKeyStates() { previousStates.swap(currentStates); }
// Return true only if the key transitioned from RELEASE to PRESS
bool isKeyJustPressed(ImGuiKey key) {
  bool current = ImGui::IsKeyDown(key);
  currentStates[key] = current;
  return !previousStates[key] && current;
}
