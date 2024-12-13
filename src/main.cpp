#include "config_app.h"
#include "editor.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imnodes/imnodes.h"
#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <graph.h>
#include <memory>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

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
  std::ostringstream log_stream;
  std::string vert_src = VERT_DEFAULT;
  std::string frag_src = FRAG_DEFAULT;
  bool frag_src_changed;
  bool zep_init = false;
  float vertices[12] = {
      -1.0f, -1.0f, 0.0f, //
      1.0f,  -1.0f, 0.0f, //
      -1.0f, 1.0f,  0.0f, //
      1.0f,  1.0f,  0.0f, //
  };
  unsigned int indices[6] = {0, 1, 2, 2, 1, 3};
  unsigned int vbo;
  unsigned int vao;
  unsigned int ebo;
  unsigned int viewport_fbo;
  unsigned int viewport_colorbuffer;
  unsigned int vert_shader;
  unsigned int frag_shader;
  unsigned int program;
};

static void glfw_error_callback(int error, const char *description) {
  spdlog::error("GLFW Error %d: %s\n", error, description);
}
void processInput(GLFWwindow *window);
// Compiles shader with shader sources defined in AppState
int compileShader(AppState *app_state);
void setupDockspace();
void renderEditor(AppState *app_state);
void renderViewport(AppState *app_state);
void renderConsole(AppState *app_state);
void renderNodeEditor(AppState *app_state);

// Main code
int main(int, char **) {
  AppState *state = new AppState();

  // Setup logger for gui and stdout
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::debug);
  auto ostream_sink =
      std::make_shared<spdlog::sinks::ostream_sink_mt>(state->log_stream);
  ostream_sink->set_level(spdlog::level::info);

  std::vector<spdlog::sink_ptr> sinks = {ostream_sink, console_sink};
  auto logger =
      std::make_shared<spdlog::logger>("Logger", sinks.begin(), sinks.end());

  spdlog::set_default_logger(logger);

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
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // ?
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable docking

  auto fontPath =
      std::filesystem::path(APP_ROOT) / "fonts" / "Cousine-Regular.ttf";
  io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 18);

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Setup Imnodes
  ImNodes::CreateContext();

  // Setup OpenGL
  glGenVertexArrays(1, &state->vao);
  glBindVertexArray(state->vao);

  glGenBuffers(1, &state->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(state->vertices), state->vertices,
               GL_STATIC_DRAW);
  glGenBuffers(1, &state->ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(state->indices), state->indices,
               GL_STATIC_DRAW);

  compileShader(state);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glGenFramebuffers(1, &state->viewport_fbo);
  glGenTextures(1, &state->viewport_colorbuffer);

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

    if (!state->zep_init) {
      // Called once the fonts/device is guaranteed setup
      zep_init(Zep::NVec2f(1.0f, 1.0f));
      zep_get_editor().InitWithText("frag.glsl", state->frag_src);
      state->zep_init = true;
    }

    zep_update();

    // Render widgets
    setupDockspace();
    renderEditor(state);
    renderViewport(state);
    renderConsole(state);
    renderNodeEditor(state);

    // Recompile Shader
    if (state->frag_src_changed) {
      spdlog::info("Recompiling Shaders!");
      state->frag_src_changed = false;
      compileShader(state);
    }

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
  glDeleteVertexArrays(1, &state->vao);
  glDeleteBuffers(1, &state->vbo);
  glDeleteBuffers(1, &state->ebo);
  glDeleteTextures(1, &state->viewport_colorbuffer);
  glDeleteFramebuffers(1, &state->viewport_fbo);

  ImNodes::DestroyContext();

  zep_destroy();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
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
void processInput(GLFWwindow *window) {
  // if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
  //   glfwSetWindowShouldClose(window, true);
  // }
  if (isKeyJustPressed(window, GLFW_KEY_F1)) {
    static bool wireframe = false;
    wireframe = !wireframe;
    if (wireframe)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
}
int compileShader(AppState *app_state) {
  int success;
  char info_log[512];
  app_state->vert_shader = glCreateShader(GL_VERTEX_SHADER);
  app_state->frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  const char *v_source = app_state->vert_src.c_str();
  const char *f_source = app_state->frag_src.c_str();
  glShaderSource(app_state->vert_shader, 1, &v_source, NULL);
  glShaderSource(app_state->frag_shader, 1, &f_source, NULL);
  glCompileShader(app_state->vert_shader);
  glCompileShader(app_state->frag_shader);
  glGetShaderiv(app_state->vert_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    spdlog::error("VERTEX Shader compilation failed!");
    glGetShaderInfoLog(app_state->vert_shader, 512, NULL, info_log);
    spdlog::error(info_log);
    return success;
  }
  glGetShaderiv(app_state->frag_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    spdlog::error("FRAGMENT Shader compilation failed!");
    glGetShaderInfoLog(app_state->frag_shader, 512, NULL, info_log);
    spdlog::error(info_log);
    return success;
  }

  app_state->program = glCreateProgram();
  glAttachShader(app_state->program, app_state->vert_shader);
  glAttachShader(app_state->program, app_state->frag_shader);
  glLinkProgram(app_state->program);
  glGetProgramiv(app_state->program, GL_COMPILE_STATUS, &success);
  if (!success) {
    spdlog::error("PROGRAM Linking process failed!");
    glGetProgramInfoLog(app_state->program, 512, NULL, info_log);
    spdlog::error(info_log);
  }
  return success;
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
void renderEditor(AppState *app_state) {
  static Zep::NVec2i size = Zep::NVec2i(640, 480);
  static uint64_t last_update = 0;

  zep_show(size);

  auto buffer = zep_get_editor().GetBuffers()[0];
  uint64_t new_update = buffer->GetUpdateCount();

  if (new_update != last_update) {
    app_state->frag_src = buffer->GetBufferText(buffer->Begin(), buffer->End());
    app_state->frag_src_changed = true;
  }

  last_update = new_update;
}
void renderViewport(AppState *app_state) {
  ImGui::Begin("Viewport");
  ImGui::BeginChild("ViewportRender");

  ImVec2 wsize = ImGui::GetWindowSize();
  glBindTexture(GL_TEXTURE_2D, app_state->viewport_colorbuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wsize.x, wsize.y, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, app_state->viewport_fbo);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         app_state->viewport_colorbuffer, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("Framebuffer is not complete!");
  }

  glViewport(0, 0, wsize.x, wsize.y);
  glClearColor(0.15f, 0.20f, 0.25f, 1.00f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(app_state->program);

  // Update Uniforms
  int u_time_location = glGetUniformLocation(app_state->program, "u_time");
  glUniform1f(u_time_location, glfwGetTime());
  int u_resolution_location =
      glGetUniformLocation(app_state->program, "u_resolution");
  glUniform2f(u_resolution_location, wsize.x, wsize.y);

  glBindVertexArray(app_state->vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  ImGui::Image((ImTextureID)app_state->viewport_colorbuffer, wsize,
               ImVec2(0, 1), ImVec2(1, 0));

  glBindFramebuffer(GL_FRAMEBUFFER, 0); // reset default framebuffer

  ImGui::EndChild();
  ImGui::End();
}
void renderConsole(AppState *app_state) {
  ImGui::Begin("Console");

  ImGui::TextWrapped("%s", app_state->log_stream.str().c_str());

  static bool sticky = false;
  float max_y = ImGui::GetScrollMaxY();
  float y = ImGui::GetScrollY();

  if (sticky)
    ImGui::SetScrollY(max_y);

  sticky = (y >= max_y);

  ImGui::End();
}
void renderNodeEditor(AppState *app_state) {
  ImGui::Begin("Node Editor");

  static RenderGraph graph;
  static bool first = true;

  {
    ImNodes::BeginNodeEditor();

    if (first) {
      FloatOutputNode out;
      graph.insert_root_node(std::make_unique<FloatOutputNode>(out));
      FloatNode node;
      graph.insert_node(std::make_unique<FloatNode>(node));

      first = false;
    }

    graph.render();

    ImNodes::EndNodeEditor();
  }

  {
    int from_pin, to_pin;
    if (ImNodes::IsLinkCreated(&from_pin, &to_pin)) {
      graph.insert_edge(from_pin, to_pin);
    }
    if (isKeyJustPressed(ImGuiKey_F2)) {
      graph.evaluate();
      auto out = dynamic_cast<FloatOutputNode *>(graph.get_root_node());
      float output = out->get_value();
      spdlog::info("Output: {}", output);
      graph.clear_graph();
    }
  }

  ImGui::End();
}
