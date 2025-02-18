#include "app.h"

#include <imgui_internal.h>
#include <toml++/toml.hpp>

void App::import_texture() {
  auto res = pfd::open_file("Select a texture file", "", {"Images", "*.png; *.jpg; *.jpg; *.tiff"})
                 .result();
  if (res.empty() || res[0].empty())
    return;
  std::filesystem::path path(res[0]);

  Texture texture(path.filename(), path);
  if (!texture)
    spdlog::error("Failed to load texture: {}", path.c_str());

  assets->insert_texture(std::make_shared<Texture>(texture));
}
void App::render_dockspace() {
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

  ImGuiDockNodeFlags docknode_flags = ImGuiDockNodeFlags_NoCloseButton |
                                      ImGuiDockNodeFlags_NoWindowMenuButton |
                                      ImGuiDockNodeFlags_PassthruCentralNode;

  if (!show_tab_bar)
    docknode_flags |= ImGuiDockNodeFlags_NoTabBar;

  // Create a dock space
  ImGuiID dockspace_id = ImGui::GetID("DockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0, 0), docknode_flags);

  ImGui::End();
}
toml::table App::try_save_toml() {

  // Save Settings
  toml::table settings{
      {"show_tab_bar", show_tab_bar},
      {"current_workspace", current_workspace},
      {"next_widget_id", next_widget_id},
      {"graph_id", graph_id},
  };

  // Save Workspaces
  toml::array workspaces{};
  for (auto &pair : this->workspaces) {
    toml::array widgets{};
    for (auto &widget : pair.second) {
      widgets.push_back(widget->save());
    }

    workspaces.push_back(toml::table{{"name", pair.first}, {"Widgets", widgets}});
  }

  return toml::table{
      {"Settings", settings},                         //
      {"Workspaces", workspaces},                     //
      {"Assets", assets->save(project_root.value())}, // Save Assets
  };
}
void App::try_load_toml(toml::table &tbl) {
  // Load Settings
  show_tab_bar = tbl["Settings"]["show_tab_bar"].value<bool>().value();
  current_workspace = tbl["Settings"]["current_workspace"].value<int>().value();
  next_widget_id = tbl["Settings"]["next_widget_id"].value<int>().value();
  graph_id = tbl["Settings"]["graph_id"].value<int>().value();

  // Load Assets
  assets = std::make_shared<AssetManager>();
  assets->load(*tbl["Assets"].as_table(), project_root.value());

  // Load Workspaces
  for (auto &n_workspace : *tbl["Workspaces"].as_array()) {
    toml::table *t_workspace = n_workspace.as_table();
    std::string name = (*t_workspace)["name"].value<std::string>().value();

    std::vector<std::shared_ptr<Widget>> widgets = {};
    for (auto &n_widget : *(*t_workspace)["Widgets"].as_array()) {
      toml::table *t_widget = n_widget.as_table();
      std::string type = (*t_widget)["type"].value<std::string>().value();

      // clang-format off
      if (type == "ConsoleWidget")
        widgets.push_back(std::make_shared<ConsoleWidget>(ConsoleWidget::load(*t_widget, assets)));
      if (type == "EditorWidget")
        widgets.push_back(std::make_shared<EditorWidget>(EditorWidget::load(*t_widget, assets)));
      if (type == "ViewportWidget")
        widgets.push_back(std::make_shared<ViewportWidget>(ViewportWidget::load(*t_widget, assets)));
      if (type == "NodeEditorWidget")
        widgets.push_back(std::make_shared<NodeEditorWidget>(NodeEditorWidget::load(*t_widget, assets)));
      if (type == "OutlinerWidget")
        widgets.push_back(std::make_shared<OutlinerWidget>(OutlinerWidget::load(*t_widget, assets)));
      // clang-format on
    }

    workspaces.push_back({name, widgets});
  }
}
