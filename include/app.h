#pragma once

#include "app_path.h"
#include "events.h"
#include "geometry.h"
#include "graph.h"
#include "portable-file-dialogs.h"
#include "shader.h"
#include "texture.h"
#include "widgets.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h> // OpenGL headers
#include <filesystem>
#include <glad/glad.h>
#include <imgui.h>
#include <optional>
#include <vector>

using Workspace = std::pair<std::string, std::vector<std::shared_ptr<Widget>>>;

struct App {
  std::shared_ptr<AssetManager> assets = std::make_shared<AssetManager>(AssetManager());
  std::vector<Workspace> workspaces;
  AssetId<RenderGraph> graph_id;
  int current_workspace = 0;
  bool show_tab_bar = true;
  bool show_status_bar = true;

  ExportImagePopup export_image = ExportImagePopup();
  std::optional<std::filesystem::path> project_root;
  std::shared_ptr<RenderGraph> graph;
  std::string status_message = "This is a status bar!";
  bool is_project_dirty = false;

  // Setup App Logic
  App() {
    ImGui::LoadIniSettingsFromDisk((getAppDir() / "assets/imgui.ini").string().c_str());

    if (auto texture = Texture("Cat", getAppDir() / "assets/textures/cat.png"))
      auto tex_id = assets->insertTexture(std::make_shared<Texture>(texture));
    else
      spdlog::error("Failed to load texture assets/textures/cat.png");

    auto shader_id = assets->insertShader(std::make_shared<Shader>("Default"));
    auto geo_id = assets->insertGeometry(std::make_shared<ScreenQuadGeometry>());

    graph = std::make_shared<RenderGraph>(RenderGraph(assets, geo_id));
    graph->default_layout(assets, shader_id);
    graph_id = assets->insertRenderGraph(graph);

    // Setup global widgets
    export_image.set_graph(graph);

    // clang-format off
    workspaces = std::vector<Workspace>({
        Workspace("Shading",
                  {
                      std::make_shared<EditorWidget>(assets->get_widget_id(), assets, shader_id),
                      std::make_shared<ViewportWidget>(assets->get_widget_id(), assets, graph_id),
                      std::make_shared<ConsoleWidget>(assets->get_widget_id()),
                      std::make_shared<OutlinerWidget>(assets->get_widget_id(), assets),
                  }),
        Workspace("RenderGraph", {std::make_shared<NodeEditorWidget>(assets->get_widget_id(), assets, graph_id)}),
    });
    // clang-format on
  }
  // Render the application
  void render() {
    render_menubar();
    render_statusbar();

    render_dockspace();
    for (auto &widget : workspaces[current_workspace].second) {
      bool open = true;
      widget->render(&open);
      if (!open)
        EventQueue::push(DeleteWidget(widget->get_id()));
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

    handle_events();

    for (auto &widget : workspaces[current_workspace].second)
      widget->onUpdate();
  }
  // Handle deferred events from EventQueue
  void handle_events();
  void process_input();
  void render_menubar();
  void render_statusbar();
  void new_shader();
  void import_texture();
  void render_dockspace();

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
  void save_project(std::filesystem::path);
  void open_project();
};
