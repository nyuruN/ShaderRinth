#include "config_app.h"
#include "portable-file-dialogs.h"
#include "widgets.h" // Will all other headers
#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <imgui.h>
#include <optional>

using Workspace = std::pair<std::string, std::vector<std::shared_ptr<Widget>>>;

struct App {
  // Assets
  std::shared_ptr<Assets<Shader>> shaders = std::make_shared<Assets<Shader>>();
  std::shared_ptr<Assets<Geometry>> geometries =
      std::make_shared<Assets<Geometry>>();
  std::shared_ptr<RenderGraph> graph;
  // GUI Layout
  std::vector<Workspace> workspaces;
  int current_workspace = 0;
  // Project
  std::optional<std::filesystem::path> project_root;

  // Setup App Logic
  App() {
    graph = std::make_shared<RenderGraph>(shaders);
    graph->set_geometry(std::make_shared<ScreenQuadGeometry>());
    auto shader =
        std::make_shared<Shader>(Shader(DEFAULT_FRAG_PATH, "Default"));
    shaders->insert(std::make_pair(shader->name, shader));

    workspaces = std::vector<Workspace>({
        Workspace("Shading",
                  {
                      std::make_shared<EditorWidget>(shader),
                      std::make_shared<ViewportWidget>(graph),
                      std::make_shared<ConsoleWidget>(),
                  }),
        Workspace("RenderGraph", {std::make_shared<NodeEditorWidget>(graph)}),
    });
  }
  // void save() {} // TODO
  // void load() {} // TODO
  void save_settings() {
    const auto path = std::filesystem::path(APP_ROOT) / "srconfig.json";
    // TODO
  }
  void load_settings() {} // TODO
  // TODO: Save everything related to a project
  void save_project() {
    if (!project_root) {
      auto dir = pfd::select_folder("Select a project directory").result();
      project_root = dir;
    }

    std::ofstream ofs(project_root.value() / "srproject.json");
    cereal::JSONOutputArchive archive(ofs);

    archive(                                               //
        VP(shaders),                                       //
        VP(geometries),                                    //
        VP(graph),                                         //
        VP(workspaces),                                    //
        VP(current_workspace),                             //
        NVP("project_root", project_root.value().string()) //
    );

    archive.serializeDeferments();

    spdlog::info("Project saved in {}!", project_root.value().string());
  }
  void load_project(std::filesystem::path project_root) {}
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
      if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("Open");
        if (ImGui::MenuItem("Save"))
          save_project();
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("View")) {
        ImGui::EndMenu();
      }

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
