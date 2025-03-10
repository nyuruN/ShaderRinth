#include "app.h"

#include <events.h>
#include <fstream>
#include <imgui_internal.h>
#include <toml++/toml.hpp>

static constexpr float STATUS_BAR_HEIGHT = 19;

void App::process_input() {
  auto io = ImGui::GetIO();
  if (isKeyJustPressed(ImGuiKey_O) && io.KeyCtrl)
    open_project();
  if (isKeyJustPressed(ImGuiKey_S) && io.KeyCtrl)
    save_project();
  if (isKeyJustPressed(ImGuiKey_N) && io.KeyCtrl)
    new_shader();
  if (isKeyJustPressed(ImGuiKey_F1)) {
    static bool wireframe = false;
    wireframe = !wireframe;
    if (wireframe)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
}
void App::handle_events() {
  while (auto if_event = EventQueue::pop()) {
    auto event = if_event.value();
    if (std::holds_alternative<AddWidget>(event)) {
      auto ev = std::get<AddWidget>(event);
      ev.widget->onStartup();
      workspaces[current_workspace].second.push_back(ev.widget);
    } else if (std::holds_alternative<DeleteWidget>(event)) {
      auto ev = std::get<DeleteWidget>(event);
      // Find widget
      auto it = std::find_if(
          workspaces[current_workspace].second.begin(), workspaces[current_workspace].second.end(),
          [ev](std::shared_ptr<Widget> &widget) { return widget->get_id() == ev.widget_id; });
      if (it == workspaces[current_workspace].second.end()) {
        spdlog::warn("Trying to delete non-existant widget!");
        continue;
      }
      it->get()->onShutdown();
      workspaces[current_workspace].second.erase(it);
    } else if (std::holds_alternative<StatusMessage>(event)) {
      auto ev = std::get<StatusMessage>(event);
      status_message = ev.message;
    }
  }
}
void App::render_menubar() {
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
      if (ImGui::MenuItem("New Shader", "Ctrl+N"))
        new_shader();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Console"))
        EventQueue::push(
            AddWidget(std::make_shared<ConsoleWidget>(ConsoleWidget(assets->get_widget_id()))));
      if (ImGui::MenuItem("Viewport"))
        EventQueue::push(AddWidget(std::make_shared<ViewportWidget>(
            ViewportWidget(assets->get_widget_id(), assets, graph_id))));
      if (ImGui::BeginMenu("Editor")) {
        for (auto &pair : *assets->getShaderCollection()) {
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

          EventQueue::push(AddWidget(std::make_shared<EditorWidget>(
              EditorWidget(assets->get_widget_id(), assets, pair.first))));
        }
        ImGui::EndMenu();
      }
      if (ImGui::MenuItem("Outliner"))
        EventQueue::push(AddWidget(
            std::make_shared<OutlinerWidget>(OutlinerWidget(assets->get_widget_id(), assets))));
      ImGui::Separator();

      ImGui::Checkbox("Show Tab Bar", &show_tab_bar);
      ImGui::Checkbox("Show Status Bar", &show_status_bar);

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
void App::render_statusbar() {
  if (!show_status_bar)
    return;

  ImGui::SetNextWindowPos({0, ImGui::GetWindowViewport()->Size.y - STATUS_BAR_HEIGHT});
  ImGui::SetNextWindowSize({ImGui::GetWindowViewport()->Size.x, STATUS_BAR_HEIGHT});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
  ImGui::Begin("Menubar", NULL,
               ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove);
  ImGui::PopStyleVar();

  const float width = ImGui::GetWindowSize().x;

  if (ImGui::BeginMenuBar()) {
    ImGui::TextUnformatted(status_message.c_str());

    // Get framerate
    static float prev = glfwGetTime();
    float curr = glfwGetTime();
    float frame = 1 / (curr - prev);
    prev = curr;

    ImGui::Indent(width - ImGui::CalcTextSize("FPS: 00.00 ").x);
    ImGui::Text("FPS: %.1f", frame);

    ImGui::EndMenuBar();
  }
  ImGui::End();
}
void App::new_shader() {
  auto shader_id = assets->insertShader(std::make_shared<Shader>(Shader("NewShader")));
  EventQueue::push(AddWidget(
      std::make_shared<EditorWidget>(EditorWidget(assets->get_widget_id(), assets, shader_id))));
  spdlog::info("Shader \"NewShader\" created!");
}
void App::import_texture() {
  auto res = pfd::open_file("Select a texture file", "", {"Images", "*.png; *.jpg; *.jpg; *.tiff"})
                 .result();
  if (res.empty() || res[0].empty())
    return;
  std::filesystem::path path(res[0]);

  Texture texture(path.filename().string(), path);
  if (!texture)
    spdlog::error("Failed to load texture: {}", path.string());

  assets->insertTexture(std::make_shared<Texture>(texture));
}
void App::render_dockspace() {
  // Create a window just below the menu to host the docking space
  float menu_height = ImGui::GetFrameHeight();
  ImVec2 size = ImGui::GetMainViewport()->Size;
  size.y -= menu_height + STATUS_BAR_HEIGHT * show_status_bar;
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

void App::save_project(std::filesystem::path project_directory) {
  toml::table tbl = try_save_toml();
  std::ofstream ofs(project_root.value() / "srproject.toml");
  ofs << tbl;

  ImGui::SaveIniSettingsToDisk((project_root.value() / "sr_imgui.ini").string().c_str());

  spdlog::info("Project saved in {}", project_root.value().string());
}
void App::open_project() {
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

    ImGui::ClearIniSettings();
    ImGui::LoadIniSettingsFromMemory(str.c_str(), str.size());
  }

  startup();

  spdlog::info("Project loaded {}", project_root.value().string());
}
toml::table App::try_save_toml() {
  // Save Settings
  toml::table settings{
      {"show_tab_bar", show_tab_bar},
      {"show_status_bar", show_status_bar},
      {"current_workspace", current_workspace},
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
  show_status_bar = tbl["Settings"]["show_status_bar"].value<bool>().value();
  current_workspace = tbl["Settings"]["current_workspace"].value<int>().value();
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

      widgets.push_back(Widget::load(*t_widget, assets));
    }

    workspaces.push_back({name, widgets});
  }
}
