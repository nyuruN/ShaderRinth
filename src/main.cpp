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

const char *VERT_DEFAULT = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)";
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
  std::string vert_src = VERT_DEFAULT;
  std::string frag_src = FRAG_DEFAULT;
  bool startup_frame = true;
  unsigned int vert_shader;
  unsigned int frag_shader;
  unsigned int program;
};

static void glfw_error_callback(int error, const char *description) {
  spdlog::error("GLFW Error %d: %s\n", error, description);
}
void processInput(GLFWwindow *window);
// Compiles shader with shader sources defined in AppState
// int compileShader(AppState *app_state);
void setupDockspace();
// void renderEditor(AppState *app_state);
// void renderViewport(AppState *app_state);
// void renderConsole(AppState *app_state);
// void renderNodeEditor(AppState *app_state);

// Main code
int main(int, char **) {
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
  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "ShaderRinth", nullptr, nullptr);
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

  // Setup App Logic
  AppState *state = new AppState();
  auto graph = std::make_shared<RenderGraph>();
  auto geo = std::make_shared<ScreenQuadGeometry>();
  auto shader = std::make_shared<Shader>(Shader((char *)FRAG_DEFAULT));
  // assets.set_geometry(std::string("ScreenQuad"), geo);
  assets.set_shader(std::string("Default"), shader);
  graph->set_geometry(geo);

  EditorWidget editor = EditorWidget("frag.glsl", FRAG_DEFAULT);
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
      spdlog::info("Begin evaluation");
      graph->evaluate();
      auto out = dynamic_cast<OutputNode *>(graph->get_root_node());
      int output = out->get_image();
      spdlog::info("Output: {}", output);
      graph->clear_graph();
      viewport.image_override = output;
    }

    // Render widgets
    setupDockspace();
    editor.render();
    viewport.render();
    console.render();
    node_editor.render();

    // Recompile Shader
    // if (editor.is_dirty) {
    //   spdlog::info("Recompiling Shaders!");
    //   editor.is_dirty = false;
    //   compileShader(state);
    // }

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
  glDeleteShader(state->vert_shader);
  glDeleteShader(state->frag_shader);
  glDeleteProgram(state->program);

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
// int compileShader(AppState *app_state) {
//   int success;
//   char info_log[512];
//   app_state->vert_shader = glCreateShader(GL_VERTEX_SHADER);
//   app_state->frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
//   const char *v_source = app_state->vert_src.c_str();
//   const char *f_source = app_state->frag_src.c_str();
//   glShaderSource(app_state->vert_shader, 1, &v_source, NULL);
//   glShaderSource(app_state->frag_shader, 1, &f_source, NULL);
//   glCompileShader(app_state->vert_shader);
//   glCompileShader(app_state->frag_shader);
//   glGetShaderiv(app_state->vert_shader, GL_COMPILE_STATUS, &success);
//   if (!success) {
//     spdlog::error("VERTEX Shader compilation failed!");
//     glGetShaderInfoLog(app_state->vert_shader, 512, NULL, info_log);
//     spdlog::error(info_log);
//     return success;
//   }
//   glGetShaderiv(app_state->frag_shader, GL_COMPILE_STATUS, &success);
//   if (!success) {
//     spdlog::error("FRAGMENT Shader compilation failed!");
//     glGetShaderInfoLog(app_state->frag_shader, 512, NULL, info_log);
//     spdlog::error(info_log);
//     return success;
//   }
//
//   app_state->program = glCreateProgram();
//   glAttachShader(app_state->program, app_state->vert_shader);
//   glAttachShader(app_state->program, app_state->frag_shader);
//   glLinkProgram(app_state->program);
//   glGetProgramiv(app_state->program, GL_COMPILE_STATUS, &success);
//   if (!success) {
//     spdlog::error("PROGRAM Linking process failed!");
//     glGetProgramInfoLog(app_state->program, 512, NULL, info_log);
//     spdlog::error(info_log);
//   }
//   return success;
// }
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
  //  | ImGuiWindowFlags_Popup;

  // Optionally disable padding and scrolling in the dockspace window
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
