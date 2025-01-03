#include "config_app.h"
#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
// #include <utils.h>
#include <widgets.h> // Will drag graph.h

struct App {
  typedef std::vector<std::shared_ptr<Widget>> Workspace;

  std::vector<Workspace> workspaces; // Tabs
  uint current_workspace = 0;

  std::shared_ptr<EditorWidget> editor;
  std::shared_ptr<NodeEditorWidget> node_editor;
  std::shared_ptr<ViewportWidget> viewport;
  std::shared_ptr<ConsoleWidget> console;

  std::shared_ptr<RenderGraph> graph;

  std::shared_ptr<Assets<Shader>> shaders = std::make_shared<Assets<Shader>>();
  std::shared_ptr<Assets<Geometry>> geometries =
      std::make_shared<Assets<Geometry>>();

  // Setup App Logic
  App() {
    graph = std::make_shared<RenderGraph>(RenderGraph(shaders));
    graph->set_geometry(std::make_shared<ScreenQuadGeometry>());
    auto shader = std::make_shared<Shader>(
        Shader(std::filesystem::path(APP_ROOT) / "assets" / "shaders" /
               "default_frag.glsl"));
    shaders->insert(std::make_pair(std::string("Default"), shader));

    editor = std::make_shared<EditorWidget>(EditorWidget(shader));
    viewport = std::make_shared<ViewportWidget>(ViewportWidget(graph));
    console = std::make_shared<ConsoleWidget>(ConsoleWidget());
    node_editor = std::make_shared<NodeEditorWidget>(NodeEditorWidget(graph));
  }
  void startup() {
    editor->onStartup();
    viewport->onStartup();
    console->onStartup();
    node_editor->onStartup();
  }
  void update() {
    editor->onUpdate();
    viewport->onUpdate();
    console->onUpdate();
    node_editor->onUpdate();
  }
  // Render widgets
  void render() {
    setupDockspace(); // Render a tab
    editor->render();
    viewport->render();
    console->render();
    node_editor->render();
  }
  // Cleanup
  void shutdown() {
    editor->onShutdown();
    viewport->onShutdown();
    console->onShutdown();
    node_editor->onShutdown();
  }
  void setupDockspace() {
    // Create a full-screen window to host the docking space
    ImGui::SetNextWindowPos(ImVec2(0, 0)); // Position at the top-left corner
    ImGui::SetNextWindowSize(
        ImGui::GetIO().DisplaySize);   // Size matches display
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
};
