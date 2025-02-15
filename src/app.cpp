#include "app.h"

#include <toml++/toml.hpp>

toml::table App::try_save_toml() {

  // Save Settings
  toml::table settings{
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
      {"Settings", settings},     //
      {"Workspaces", workspaces}, //
      {"Assets", assets->save()}, // Save Assets
  };
}
void App::try_load_toml(toml::table &tbl) {
  // Load Settings
  current_workspace = tbl["Settings"]["current_workspace"].value<int>().value();
  next_widget_id = tbl["Settings"]["next_widget_id"].value<int>().value();
  graph_id = tbl["Settings"]["graph_id"].value<int>().value();

  // Load Assets
  assets = std::make_shared<AssetManager>();
  assets->load(*tbl["Assets"].as_table());

  spdlog::info("Assets loaded");

  // Load Workspaces
  for (auto &n_workspace : *tbl["Workspaces"].as_array()) {
    toml::table *t_workspace = n_workspace.as_table();
    std::string name = (*t_workspace)["name"].value<std::string>().value();

    spdlog::info("Loading Workspace {}", name);

    std::vector<std::shared_ptr<Widget>> widgets = {};
    for (auto &n_widget : *(*t_workspace)["Widgets"].as_array()) {
      toml::table *t_widget = n_widget.as_table();
      std::string type = (*t_widget)["type"].value<std::string>().value();

      spdlog::info("Loading Widget {}", type);

      // clang-format off
      if (type == "ConsoleWidget")
        widgets.push_back(std::make_shared<ConsoleWidget>(ConsoleWidget::load(*t_widget)));
      if (type == "EditorWidget")
        widgets.push_back(std::make_shared<EditorWidget>(EditorWidget::load(*t_widget, assets)));
      if (type == "ViewportWidget")
        widgets.push_back(std::make_shared<ViewportWidget>(ViewportWidget::load(*t_widget, assets)));
      if (type == "NodeEditorWidget")
        widgets.push_back(std::make_shared<NodeEditorWidget>(NodeEditorWidget::load(*t_widget, assets)));
      // clang-format on
    }

    workspaces.push_back({name, widgets});
  }
}
