#pragma once

#include "config_app.h"
#include "geometry.h"
#include "graph.h"
#include "shader.h"
#include "texture.h"
#include "widgets.h"
#include <GLFW/glfw3.h> // OpenGL headers
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <optional>
#include <portable-file-dialogs.h>
#include <vector>

#include <cereal/cereal.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>

using Workspace = std::pair<std::string, std::vector<std::shared_ptr<Widget>>>;

struct App {
  // clang-format off
  std::shared_ptr<Assets<Texture>>      textures = std::make_shared<Assets<Texture>>();
  std::shared_ptr<Assets<Shader>>       shaders = std::make_shared<Assets<Shader>>();
  std::shared_ptr<Assets<Geometry>>     geometries = std::make_shared<Assets<Geometry>>();
  std::shared_ptr<RenderGraph>          graph;
  std::vector<Workspace>                workspaces;
  int                                   current_workspace = 0;
  int                                   next_widget_id = 0;
  // clang-format on

  std::optional<std::filesystem::path> project_root;
  ExportImagePopup export_image = ExportImagePopup();
  std::vector<std::shared_ptr<Widget>> deferred_add_widget;
  bool is_project_dirty = false;

  // Setup App Logic
  App() {
    ImGui::LoadIniSettingsFromDisk(
        (std::filesystem::path(APP_ROOT) / "assets/imgui.ini").c_str());

    auto shader = std::make_shared<Shader>(Shader("Default"));
    auto geo = std::make_shared<ScreenQuadGeometry>();
    auto texture = std::make_shared<Texture>(
        "Cat", std::filesystem::path(APP_ROOT) / "assets/textures/cat.png");

    if (!*texture)
      spdlog::error("Failed to load texture assets/textures/cat.png");

    shaders->insert(std::make_pair(shader->get_name(), shader));
    geometries->insert(std::make_pair(geo->get_name(), geo));
    textures->insert(std::make_pair(texture->get_name(), texture));
    graph = std::make_shared<RenderGraph>(shaders, textures, geo);
    graph->default_layout(shader);
    export_image.set_graph(graph);

    // clang-format off
    workspaces = std::vector<Workspace>({
        Workspace("Shading",
                  {
                      std::make_shared<EditorWidget>(shader),
                      std::make_shared<ViewportWidget>(next_widget_id++, graph),
                      std::make_shared<ConsoleWidget>(next_widget_id++),
                  }),
        Workspace("RenderGraph", {std::make_shared<NodeEditorWidget>(graph)}),
    });
    // clang-format on
  }
  // Render the application
  void render() {
    render_menubar();

    render_dockspace();
    for (auto &widget : workspaces[current_workspace].second)
      widget->render();

    export_image.render();
  }
  void startup() {
    for (auto &pair : workspaces) {
      for (auto &widget : pair.second)
        widget->onStartup();
    }
    export_image.onStartup();
  }
  void shutdown() {
    for (auto &pair : workspaces) {
      for (auto &widget : pair.second)
        widget->onShutdown();
    }
    for (auto &pair : *textures) {
      pair.second->destroy();
    }
    for (auto &pair : *shaders) {
      pair.second->destroy();
    }
    export_image.onShutdown();
    workspaces.clear();
    graph.reset();
    geometries.reset();
    shaders.reset();
    textures.reset();
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
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Console"))
          deferred_add_widget.push_back(
              std::make_shared<ConsoleWidget>(next_widget_id++));
        if (ImGui::MenuItem("Viewport"))
          deferred_add_widget.push_back(
              std::make_shared<ViewportWidget>(next_widget_id++, graph));

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
    Global::instance().project_root = project_root.value();
    save_project(project_root.value());
  }
  // Save everything related to a project
  void save_project(std::filesystem::path project_directory) {
    std::ofstream ofs(project_root.value() / "srproject.json");
    cereal::JSONOutputArchive archive(ofs);

    archive(                   //
        VP(shaders),           //
        VP(geometries),        //
        VP(textures),          //
        VP(graph),             //
        VP(workspaces),        //
        VP(current_workspace), //
        VP(next_widget_id)     //
    );

    ImGui::SaveIniSettingsToDisk(
        (project_root.value() / "sr_imgui.ini").c_str());

    spdlog::info("Project saved in {}", project_root.value().string());
  }
  void open_project() {
    if (is_project_dirty) {
      // TODO: Prompt the user to save the project first
    }

    auto dir_str =
        pfd::select_folder("Select an existing project directory").result();
    if (dir_str.empty())
      return;

    std::ifstream file(std::filesystem::path(dir_str) / "srproject.json");
    if (!file) {
      spdlog::error("Unknown project directory format!");
      return;
    }
    project_root = dir_str;
    Global::instance().project_root = project_root.value();

    // Safely clear all resources
    shutdown();

    {
      cereal::JSONInputArchive archive(file);
      archive(                   //
          VP(shaders),           //
          VP(geometries),        //
          VP(textures),          //
          VP(graph),             //
          VP(workspaces),        //
          VP(current_workspace), //
          VP(next_widget_id)     //
      );
    }

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
    export_image.set_graph(graph);

    spdlog::info("Project loaded {}", project_root.value().string());
  }
  void import_texture() {
    auto res = pfd::open_file("Select a texture file", "",
                              {"Images", "*.png; *.jpg; *.jpg; *.tiff"})
                   .result();
    if (res.empty() || res[0].empty())
      return;
    std::filesystem::path path(res[0]);

    Texture texture(path.filename(), path);
    if (!texture)
      spdlog::error("Failed to load texture: {}", path.c_str());

    textures->insert(
        std::make_pair(texture.get_name(), std::make_shared<Texture>(texture)));
  }
  void render_dockspace() {
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
