#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h> // Defines OpenGL headers
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include "IconsFontAwesome6.h"
#include "app.h"
#include "editor.h"
#include "theme.h"

static void glfw_error_callback(int error, const char *description) {
  spdlog::error("GLFW Error %d: %s\n", error, description);
}
static void zep_message_callback(std::shared_ptr<Zep::ZepMessage> message) {
  if (message->messageId == Zep::Msg::GetClipBoard) {
    message->str =
        std::string(ImGui::GetPlatformIO().Platform_GetClipboardTextFn(ImGui::GetCurrentContext()));
    message->handled = true;
  }
  if (message->messageId == Zep::Msg::SetClipBoard) {
    std::string str = std::string(message->str);
    ImGui::GetPlatformIO().Platform_SetClipboardTextFn(ImGui::GetCurrentContext(), str.c_str());
    message->handled = true;
  }
}

// Main code
int main(int, char **) {
  GLFWwindow *window;
  { // Setup GLFW, ImGui Zep etc.
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
    if (window == nullptr) {
      std::cerr << "Failed to initialize GLFW" << std::endl;
      return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cerr << "Failed to initialize GLAD" << std::endl;
      return 1;
    }

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsCinder();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr; // Disable automatic saves
    // Enable standard controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable; // Enable docking

    ImNodes::CreateContext();

    { // Load fonts
      io.Fonts->ClearFonts();
      static const float font_size = 15.0f;
      auto fontPath = getAppDir() / "assets/fonts/Cousine-Regular.ttf";
      io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), font_size);

      static const ImWchar FA_ICON_RANGES[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
      float icon_size = round(font_size * 0.9f);
      ImFontConfig config;
      config.MergeMode = true;
      config.GlyphMinAdvanceX = icon_size; // Make the icons monospace
      auto faPath = getAppDir() / "assets/fonts/fontawesome-free-6.7.2-web/webfonts/" /
                    FONT_ICON_FILE_NAME_FAR;
      io.Fonts->AddFontFromFileTTF(faPath.string().c_str(), icon_size, &config, FA_ICON_RANGES);
      auto fasPath = getAppDir() / "assets/fonts/fontawesome-free-6.7.2-web/webfonts/" /
                     FONT_ICON_FILE_NAME_FAS;
      io.Fonts->AddFontFromFileTTF(fasPath.string().c_str(), icon_size, &config, FA_ICON_RANGES);
      io.Fonts->Build();
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
  }

  Global::instance().init();
  App app = App();

  // Main loop
  bool first_frame = true;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

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

      zep_init(Zep::NVec2f(1.0f, 1.0f), zep_message_callback);
      ZepStyleColorsCinder();
      auto &config = zep_get_editor().GetConfig();
      config.showTabBar = false;
      config.autoHideAirlineRegion = false;
      config.autoHideCommandRegion = true;
      config.searchGitRoot = false;

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
  Global::instance().shutdown();

  ImNodes::DestroyContext();
  zep_destroy();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
