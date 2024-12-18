#include <GLFW/glfw3.h>
#include <any>
#include <imgui.h>
#include <map>
#include <spdlog/spdlog.h>

void debugAny(std::any &value) {
  if (value.has_value()) {
    spdlog::info("Stored type: {}", value.type().name());
  } else {
    spdlog::info("std::any is empty");
  }
}
// Return true only if the key transitioned from RELEASE to PRESS
bool isKeyJustPressed(GLFWwindow *window, int key) {
  static std::map<int, bool> keyPrevState;
  int currentState = glfwGetKey(window, key);
  bool prevState = keyPrevState[key];

  keyPrevState[key] = (currentState == GLFW_PRESS);

  return !prevState && (currentState == GLFW_PRESS);
}
// Return true only if the key transitioned from RELEASE to PRESS
// (ImGui version)
bool isKeyJustPressed(ImGuiKey key) {
  static std::map<ImGuiKey, bool> keyPrevState;
  bool currentState = ImGui::IsKeyDown(key);
  bool prevState = keyPrevState[key];

  keyPrevState[key] = currentState;

  return !prevState && currentState;
}
