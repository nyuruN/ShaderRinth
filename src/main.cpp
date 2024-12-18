#include "config_app.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imnodes/imnodes.h"
#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
// #include <utils.h>
#include <widgets.h> // Will drag graph.h

const char *FRAG_DEFAULT = R"(
#version 330 core

out vec4 FragColor;

uniform float u_time;
uniform vec2 u_resolution;

void main() {
  vec3 col = vec3(0.5f);
  vec2 uv = gl_FragCoord.xy / u_resolution;

  col.xy = uv;
  col.z = sin(u_time) * 0.5f + 0.5f;

  FragColor = vec4(col, 1.0f);
}
)";

struct AppState {
  std::string frag_src = FRAG_DEFAULT;
  bool startup_frame = true;
};

static void glfw_error_callback(int error, const char *description) {
  spdlog::error("GLFW Error %d: %s\n", error, description);
}
void processInput(GLFWwindow *window);
void setupDockspace();

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

    auto fontPath =
        std::filesystem::path(APP_ROOT) / "fonts" / "Cousine-Regular.ttf";
    io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 16);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
  }

  // Setup App Logic
  AppState *state = new AppState();
  auto graph = std::make_shared<RenderGraph>();
  auto geo = std::make_shared<ScreenQuadGeometry>();
  auto shader = std::make_shared<Shader>(Shader((char *)FRAG_DEFAULT));
  assets.set_shader(std::string("Default"), shader);
  graph->set_geometry(geo);

  EditorWidget editor = EditorWidget("default.glsl", FRAG_DEFAULT);
  ViewportWidget viewport = ViewportWidget();
  ConsoleWidget console = ConsoleWidget();
  NodeEditorWidget node_editor = NodeEditorWidget(graph);

  // Main loop
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

    if (state->startup_frame) {
      state->startup_frame = false;
      editor.onStartup();
      viewport.onStartup();
      console.onStartup();
      node_editor.onStartup();
    }

    editor.onUpdate();
    viewport.onUpdate();
    console.onUpdate();
    node_editor.onUpdate();

    if (isKeyJustPressed(ImGuiKey_F2)) {
      graph->evaluate();
      auto out = dynamic_cast<OutputNode *>(graph->get_root_node());
      int output = out->get_image();
      viewport.set_image(output);
      graph->clear_graph_data();
    }

    // Render widgets
    setupDockspace();
    editor.render();
    viewport.render();
    console.render();
    node_editor.render();

    // TODO: Recompile Shader

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

  // Cleanup
  editor.onShutdown();
  viewport.onShutdown();
  console.onShutdown();
  node_editor.onShutdown();

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
}
void setupDockspace() {
  // Create a full-screen window to host the docking space
  ImGui::SetNextWindowPos(ImVec2(0, 0)); // Position at the top-left corner
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize); // Size matches display
  ImGui::SetNextWindowBgAlpha(0.0f); // Make it invisible if desired

  // Create a window for the docking space (no title bar, resize, or move)
  ImGuiWindowFlags dockspace_flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  // Disable padding and scrolling in the dockspace window
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("DockSpaceWindow", nullptr, dockspace_flags);
  ImGui::PopStyleVar();

  // Create a dock space
  ImGuiID dockspace_id = ImGui::GetID("DockSpace");
  ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0., 0., 0., 0.));
  ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_None);
  ImGui::PopStyleColor();

  ImGui::End();
}
