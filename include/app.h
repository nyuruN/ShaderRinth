#include "config_app.h"
#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <imgui.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
// #include <utils.h>
#include <widgets.h> // Will drag graph.h

struct App {
  using Workspace =
      std::pair<std::string, std::vector<std::shared_ptr<Widget>>>;

  std::vector<Workspace> workspaces;
  int current_workspace = 0;

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
    shader->set_name("Default");

    auto editor = std::make_shared<EditorWidget>(EditorWidget(shader));
    auto viewport = std::make_shared<ViewportWidget>(ViewportWidget(graph));
    auto console = std::make_shared<ConsoleWidget>(ConsoleWidget());
    auto node_editor =
        std::make_shared<NodeEditorWidget>(NodeEditorWidget(graph));

    workspaces = std::vector<Workspace>({
        Workspace("Shading",
                  {
                      editor,
                      viewport,
                      console,
                  }),
        Workspace("RenderGraph", {node_editor}),
    });
  }
  void save() {} // TODO
  void load() {} // TODO
  void save_settings() {
    const auto path = std::filesystem::path(APP_ROOT) / "srconfig.json";
    // TODO
  }
  void load_settings() {} // TODO
  void save_project() {
    // TODO: Save everything related to a project
  }
  void load_project() {} // TODO
  void startup() {
    for (auto &pair : workspaces) {
      for (auto &widget : pair.second)
        widget->onStartup();
    }
  }
  void update() {
    for (auto &widget : workspaces[current_workspace].second)
      widget->onUpdate();
  }
  // Render the application
  void render() {
    if (ImGui::BeginMainMenuBar()) {
      // Dummy items
      ImGui::MenuItem("File");
      ImGui::MenuItem("Edit");
      ImGui::MenuItem("View");

      ImGui::Indent(165);
      if (ImGui::BeginTabBar("Workspaces", ImGuiTabBarFlags_Reorderable)) {
        int idx = 0;
        for (auto &pair : workspaces) {
          if (ImGui::BeginTabItem(pair.first.c_str())) {
            current_workspace = idx;
            ImGui::EndTabItem();
          }
          idx++;
        }
        ImGui::EndTabBar();
      }

      ImGui::EndMainMenuBar();
    }

    renderDockspace();
    for (auto &widget : workspaces[current_workspace].second)
      widget->render();
  }
  // Cleanup
  void shutdown() {
    for (auto &pair : workspaces) {
      for (auto &widget : pair.second)
        widget->onShutdown();
    }
  }
  void renderDockspace() {
    // Create a window just below the menu to host the docking space
    float menu_height = ImGui::GetFrameHeight();
    ImVec2 size = ImGui::GetWindowSize();
    size.y -= menu_height;
    ImGui::SetNextWindowPos(ImVec2(0, menu_height));
    ImGui::SetNextWindowSize(size); // Size matches display

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
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0),
                     ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
  }
};
