#pragma once

#include "config_app.h"
#include "portable-file-dialogs.h"
#include "widgets.h" // Will all other headers
#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <cereal/cereal.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <imgui.h>
#include <optional>
#include <vector>

using Workspace = std::pair<std::string, std::vector<std::shared_ptr<Widget>>>;

struct App {
  // clang-format off
  std::shared_ptr<Assets<Texture>>      textures = std::make_shared<Assets<Texture>>();
  std::shared_ptr<Assets<Shader>>       shaders = std::make_shared<Assets<Shader>>();
  std::shared_ptr<Assets<Geometry>>     geometries = std::make_shared<Assets<Geometry>>();
  std::shared_ptr<RenderGraph>          graph;
  std::vector<Workspace>                workspaces;
  int                                   current_workspace = 0;
  std::optional<std::filesystem::path>  project_root;
  // clang-format on

  bool is_project_dirty = false;

  // Setup App Logic
  App() {
    Global::instance().init();
    auto shader = std::make_shared<Shader>(Shader("Default"));
    auto geo = std::make_shared<ScreenQuadGeometry>();
    auto texture = std::make_shared<Texture>(
        "Cat", std::filesystem::path(APP_ROOT) / "assets/textures/cat.png");

    if (!texture)
      spdlog::error("Failed to load texture assets/textures/cat.png");

    shaders->insert(std::make_pair(shader->name, shader));
    geometries->insert(std::make_pair(geo->name, geo));
    textures->insert(std::make_pair(texture->get_name(), texture));
    graph = std::make_shared<RenderGraph>(shaders, textures, geo);
    graph->default_layout(shader);

    // clang-format off
    workspaces = std::vector<Workspace>({
        Workspace("Shading",
                  {
                      std::make_shared<EditorWidget>(shader),
                      std::make_shared<ViewportWidget>(graph),
                      std::make_shared<ConsoleWidget>(),
                  }),
        Workspace("RenderGraph", {std::make_shared<NodeEditorWidget>(graph)}),
    });
    // clang-format on
  }
  // TODO: Save persistent preferences global to all instances
  void save_settings() {}
  // TODO: Load persistent preferences global to all instances
  void load_settings() {}
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
    Global::instance().set_project_root(project_root.value());
    save_project(project_root.value());
  }
  // Save everything related to a project
  void save_project(std::filesystem::path project_directory) {
    std::ofstream ofs(project_root.value() / "srproject.json");
    cereal::JSONOutputArchive archive(ofs);

    archive(                  //
        VP(shaders),          //
        VP(geometries),       //
        VP(textures),         //
        VP(graph),            //
        VP(workspaces),       //
        VP(current_workspace) //
    );

    spdlog::info("Project saved in {}", project_root.value().string());
  }
  void open_project() {
    if (is_project_dirty) {
      // TODO: Prompt the user to save the project first
    }

    auto dir =
        pfd::select_folder("Select an existing project directory").result();
    if (dir.empty())
      return;

    std::ifstream file(std::filesystem::path(dir) / "srproject.json");
    if (!file) {
      spdlog::error("Unknown project directory format!");
      return;
    }
    project_root = dir;
    Global::instance().set_project_root(project_root.value());

    { // Safely clear all resources
      shutdown();
      for (auto &pair : *textures) {
        pair.second->destroy();
      }
      workspaces = {};
      graph = nullptr;
      geometries = nullptr;
      shaders = nullptr;
      textures = nullptr;
    }

    {
      cereal::JSONInputArchive archive(file);
      archive(                  //
          VP(shaders),          //
          VP(geometries),       //
          VP(textures),         //
          VP(graph),            //
          VP(workspaces),       //
          VP(current_workspace) //
      );
      startup();
    }

    spdlog::info("Project loaded {}", project_root.value().string());
  }
  void import_texture() {
    auto res = pfd::open_file("Select a texture file", "",
                              {"Images", "*.png; *.jpeg; *.jpg; *.tiff"})
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
  void startup() {
    for (auto &pair : workspaces) {
      for (auto &widget : pair.second)
        widget->onStartup();
    }
  }
  void shutdown() {
    for (auto &pair : workspaces) {
      for (auto &widget : pair.second)
        widget->onShutdown();
    }
  }
  void update() {
    updateKeyStates();
    process_input();
    for (auto &widget : workspaces[current_workspace].second)
      widget->onUpdate();
  }
  // Render the application
  void render() {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open"))
          open_project();
        if (ImGui::MenuItem("Save"))
          save_project();
        if (ImGui::MenuItem("Save as"))
          save_project_as();
        if (ImGui::MenuItem("Import texture"))
          import_texture();
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
  void process_input() {
    if (isKeyJustPressed(ImGuiKey_F1)) {
      static bool wireframe = false;
      wireframe = !wireframe;
      spdlog::info("toggle wireframe: {}", wireframe);
      if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  }
};
