#pragma once

#include "config_app.h"
#include "geometry.h"
#include "graph.h"
#include "shader.h"
#include "texture.h"
#include "widgets.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h> // OpenGL headers
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <optional>
#include <portable-file-dialogs.h>
#include <vector>

using Workspace = std::pair<std::string, std::vector<std::shared_ptr<Widget>>>;

struct App {
  std::shared_ptr<AssetManager> assets = std::make_shared<AssetManager>();
  AssetId<RenderGraph> graph_id;
  std::vector<Workspace> workspaces;
  int current_workspace = 0;
  int next_widget_id = 0;

  std::shared_ptr<RenderGraph> graph;
  std::optional<std::filesystem::path> project_root;
  ExportImagePopup export_image = ExportImagePopup();
  std::vector<std::shared_ptr<Widget>> deferred_add_widget;
  std::vector<std::shared_ptr<Widget>> deferred_remove_widget;
  bool is_project_dirty = false;

  // Setup App Logic
  App() {
    ImGui::LoadIniSettingsFromDisk((std::filesystem::path(APP_ROOT) / "assets/imgui.ini").c_str());

    auto texture = std::make_shared<Texture>("Cat", std::filesystem::path(APP_ROOT) /
                                                        "assets/textures/cat.png");

    if (!texture->is_loaded())
      spdlog::error("Failed to load texture assets/textures/cat.png");

    auto shader_id = assets->insert_shader(std::make_shared<Shader>("Default"));
    auto geo_id = assets->insert_geometry(std::make_shared<ScreenQuadGeometry>());
    auto tex_id = assets->insert_texture(texture);

    graph_id = assets->insert_graph(std::make_shared<RenderGraph>(assets, geo_id));
    graph = assets->get_graph(graph_id);
    graph->default_layout(assets, shader_id);

    // Setup global widgets
    export_image.set_graph(graph);

    // clang-format off
    workspaces = std::vector<Workspace>({
        Workspace("Shading",
                  {
                      std::make_shared<EditorWidget>(next_widget_id++, assets, shader_id),
                      std::make_shared<ViewportWidget>(next_widget_id++, assets, graph_id),
                      std::make_shared<ConsoleWidget>(next_widget_id++),
                      std::make_shared<OutlinerWidget>(next_widget_id++, assets),
                  }),
        Workspace("RenderGraph", {std::make_shared<NodeEditorWidget>(next_widget_id++, assets, graph_id)}),
    });
    // clang-format on
  }
  // Render the application
  void render() {
    render_menubar();

    render_dockspace();
    for (auto &widget : workspaces[current_workspace].second) {
      bool open = true;
      widget->render(&open);
      if (!open)
        deferred_remove_widget.push_back(widget);
    }

    export_image.render();
  }
  void startup() {
    for (auto &pair : workspaces) {
      for (auto &widget : pair.second)
        widget->onStartup();
    }
    export_image.set_graph(graph);
    export_image.onStartup();
  }
  void shutdown() {
    for (auto &pair : workspaces) {
      for (auto &widget : pair.second)
        widget->onShutdown();
    }
    assets->destroy();
    export_image.onShutdown();
    workspaces.clear();
    graph.reset();
  }
  void update() {
    updateKeyStates();
    process_input();

    if (!deferred_add_widget.empty()) {
      for (auto &widget : deferred_add_widget) {
        widget->onStartup();
        workspaces[current_workspace].second.push_back(widget);
      }
      deferred_add_widget.clear();
    }
    if (!deferred_remove_widget.empty()) {
      for (auto &widget : deferred_remove_widget) {
        widget->onShutdown();
        auto it = std::find(workspaces[current_workspace].second.begin(),
                            workspaces[current_workspace].second.end(), widget);
        workspaces[current_workspace].second.erase(it);
      }
      deferred_remove_widget.clear();
    }

    for (auto &widget : workspaces[current_workspace].second)
      widget->onUpdate();
  }
  void process_input() {
    auto io = ImGui::GetIO();
    if (isKeyJustPressed(ImGuiKey_O) && io.KeyCtrl)
      open_project();
    if (isKeyJustPressed(ImGuiKey_S) && io.KeyCtrl)
      save_project();
    if (isKeyJustPressed(ImGuiKey_F1)) {
      static bool wireframe = false;
      wireframe = !wireframe;
      if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  }
  void render_menubar() {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open", "Ctrl+O"))
          open_project();
        if (ImGui::MenuItem("Save", "Ctrl+S"))
          save_project();
        if (ImGui::MenuItem("Save As"))
          save_project_as();
        if (ImGui::MenuItem("Import Texture"))
          import_texture();
        if (ImGui::MenuItem("Export Image"))
          export_image.open_popup();

        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("New Shader")) {
          assets->insert_shader(std::make_shared<Shader>(Shader("NewShader")));
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Console"))
          deferred_add_widget.push_back(
              std::make_shared<ConsoleWidget>(ConsoleWidget(next_widget_id++)));
        if (ImGui::MenuItem("Viewport"))
          deferred_add_widget.push_back(
              std::make_shared<ViewportWidget>(ViewportWidget(next_widget_id++, assets, graph_id)));
        if (!assets->shaders->empty())
          if (ImGui::BeginMenu("Editor")) {
            for (auto &pair : *assets->shaders) {
              ImGui::PushID(pair.first);
              bool clicked = ImGui::MenuItem(pair.second->get_name().c_str());
              ImGui::PopID();

              if (!clicked)
                continue;

              bool has_editor = false;
              for (auto widget : workspaces[current_workspace].second) {
                auto editor = dynamic_cast<EditorWidget *>(widget.get());
                if (editor && editor->get_shader() == pair.first)
                  has_editor = true;
              }
              if (has_editor)
                continue;

              deferred_add_widget.push_back(std::make_shared<EditorWidget>(
                  EditorWidget(next_widget_id++, assets, pair.first)));
            }
            ImGui::EndMenu();
          }
        if (ImGui::MenuItem("Outliner"))
          deferred_add_widget.push_back(
              std::make_shared<OutlinerWidget>(OutlinerWidget(next_widget_id++, assets)));

        ImGui::EndMenu();
      }
      ImGui::Indent(165);

      ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 0.0f);
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
      ImGui::PopStyleVar();

      ImGui::EndMainMenuBar();
    }
  }
  // Default save
  void save_project() {
    if (!project_root) {
      save_project_as();
      return;
    }
    save_project(project_root.value());
  }
  // Explicitly ask for project directory
  void save_project_as() {
    auto dir = pfd::select_folder("Select a project directory").result();
    if (dir.empty())
      return;
    project_root = dir;
    save_project(project_root.value());
  }
  // Save everything related to a project
  void save_project(std::filesystem::path project_directory) {
    toml::table tbl = try_save_toml();
    std::ofstream ofs(project_root.value() / "srproject.toml");
    ofs << tbl;

    ImGui::SaveIniSettingsToDisk((project_root.value() / "sr_imgui.ini").c_str());

    spdlog::info("Project saved in {}", project_root.value().string());
  }
  void open_project() {
    if (is_project_dirty) {
      // TODO: Prompt the user to save the project first
    }

    auto dir_str = pfd::select_folder("Select an existing project directory").result();
    if (dir_str.empty())
      return;

    std::ifstream file(std::filesystem::path(dir_str) / "srproject.toml");
    if (!file) {
      spdlog::error("Unknown project directory format!");
      return;
    }
    project_root = dir_str;

    // Safely clear all resources
    shutdown();

    toml::table tbl = toml::parse(file);
    try_load_toml(tbl);

    // Load ImGui settings
    auto imgui_filepath = project_root.value() / "sr_imgui.ini";
    if (std::filesystem::exists(imgui_filepath)) {
      std::ifstream imgui_file(imgui_filepath);

      if (!imgui_file) {
        spdlog::error("Failed to load ImGui layout!");
      }

      std::ostringstream buf;
      buf << imgui_file.rdbuf();
      std::string str = buf.str();
      ImGui::LoadIniSettingsFromMemory(str.c_str(), str.size());
    }

    startup();

    spdlog::info("Project loaded {}", project_root.value().string());
  }
  void import_texture() {
    auto res =
        pfd::open_file("Select a texture file", "", {"Images", "*.png; *.jpg; *.jpg; *.tiff"})
            .result();
    if (res.empty() || res[0].empty())
      return;
    std::filesystem::path path(res[0]);

    Texture texture(path.filename(), path);
    if (!texture)
      spdlog::error("Failed to load texture: {}", path.c_str());

    assets->insert_texture(std::make_shared<Texture>(texture));
  }
  void render_dockspace() {
    // Create a window just below the menu to host the docking space
    float menu_height = ImGui::GetFrameHeight();
    ImVec2 size = ImGui::GetWindowSize();
    size.y -= menu_height;
    ImGui::SetNextWindowPos(ImVec2(0, menu_height));
    ImGui::SetNextWindowSize(size); // Size matches display

    // Create a window for the docking space (no title bar, resize, or move)
    ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus |
                                       ImGuiWindowFlags_NoNavFocus;

    // Disable padding and scrolling in the dockspace window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("DockSpaceWindow", nullptr, dockspace_flags);
    ImGui::PopStyleVar();

    // Create a dock space
    ImGuiID dockspace_id = ImGui::GetID("DockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
  }
  toml::table try_save_toml();
  void try_load_toml(toml::table &);
};
