#pragma once

#include "config_app.h"
#include "events.h"
#include "geometry.h"
#include "graph.h"
#include "shader.h"
#include "texture.h"
#include "widgets.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h> // OpenGL headers
#include <filesystem>
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
  bool show_tab_bar = true;

  std::shared_ptr<RenderGraph> graph;
  std::optional<std::filesystem::path> project_root;
  ExportImagePopup export_image = ExportImagePopup();
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
  void save_project(std::filesystem::path project_directory);
  void open_project();
  toml::table try_save_toml();
  void try_load_toml(toml::table &);
};
