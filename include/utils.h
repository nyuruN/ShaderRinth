#include <GLFW/glfw3.h>
#include <imgui.h>
#include <map>
#include <material.h>
#include <string>

template <typename T> using Assets = std::map<std::string, std::shared_ptr<T>>;

// Return true only if the key transitioned from RELEASE to PRESS
bool isKeyJustPressed(GLFWwindow *window, int key);
// Return true only if the key transitioned from RELEASE to PRESS
// (ImGui version)
bool isKeyJustPressed(ImGuiKey key);
