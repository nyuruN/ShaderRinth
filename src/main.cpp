#include "app.h"
#include "config_app.h"
#include "editor.h"
#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

static void glfw_error_callback(int error, const char *description) {
  spdlog::error("GLFW Error %d: %s\n", error, description);
}
void processInput(GLFWwindow *window);

// Main code
int main(int, char **) {
  GLFWwindow *window;
  { // Setup GLFW, ImGui etc.
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
      return 1;

    // GL 3.0 + GLSL 330
    const char *glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // Disable all deprecated features
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Create window with graphics context
    window = glfwCreateWindow(1280, 720, "ShaderRinth", nullptr, nullptr);
    if (window == nullptr)
      return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO &io = ImGui::GetIO();
    // Enable standard controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
                      ImGuiConfigFlags_DockingEnable; // Enable docking

    ImNodes::CreateContext();

    auto fontPath = std::filesystem::path(APP_ROOT) / "assets" / "fonts" /
                    "Cousine-Regular.ttf";
    io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 16);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
  }

  App app = App();

  // Main loop
  bool first_frame = true;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    processInput(window);

    // Sleep if minimized
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
      ImGui_ImplGlfw_Sleep(10);
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (first_frame) {
      first_frame = false;
      app.startup();
    }

    app.update();

    app.render();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.15f, 0.20f, 0.25f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  app.shutdown();

  ImNodes::DestroyContext();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

void processInput(GLFWwindow *window) {
  if (isKeyJustPressed(window, GLFW_KEY_F1)) {
    static bool wireframe = false;
    wireframe = !wireframe;
    spdlog::info("toggle wireframe: {}", wireframe);
    if (wireframe)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  if (isKeyJustPressed(window, GLFW_KEY_F3)) {
    auto buffer = zep_get_editor().GetBuffers()[0];
    spdlog::info("Name: {}, {}", buffer->GetName(), buffer->GetDisplayName());
  }
}
