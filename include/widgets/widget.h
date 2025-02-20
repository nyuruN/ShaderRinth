#pragma once

#include <functional>
#include <imgui.h>
#include <unordered_map>

#include <toml++/toml.hpp>

#define REGISTER_WIDGET_FACTORY(type)                                                              \
  namespace {                                                                                      \
  static bool reg_##type = (WidgetFactory::register_factory(#type, type::load), true);             \
  }

// Forward declares
class Widget;
struct AssetManager;

// Widgets should implement a factory function as follows:
//
// static std::shared_ptr<Widget> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
//   // Deserialization code
// }
struct WidgetFactory {
public:
  using Factory =
      std::function<std::shared_ptr<Widget>(toml::table &, std::shared_ptr<AssetManager>)>;

  static void register_factory(std::string name, Factory factory) {
    instance().factories.insert({name, factory});
  }
  static Factory get(std::string name) { return instance().factories.at(name); }

private:
  std::unordered_map<std::string, Factory> factories;

  static WidgetFactory &instance() {
    static WidgetFactory factory;
    return factory;
  }
};

// A stateful widget
class Widget {
protected:
  int id = -1;

public:
  // Runs on the first frame when loaded
  virtual void onStartup() {};
  // Runs on shutdown or otherwise destroyed
  virtual void onShutdown() {};
  // Runs every frame
  virtual void onUpdate() {};
  // Runs every frame if visible
  // Setting p_open to false will close the widget
  virtual void render(bool *p_open) {};

  virtual toml::table save() = 0;
  static std::shared_ptr<Widget> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    std::string type = tbl["type"].value<std::string>().value();
    return WidgetFactory::get(type)(tbl, assets);
  }
};

// Should not be serialized
class PopupWidget : Widget {
protected:
  bool is_open = false;

  // Should be called at the start of the render function
  // MUST BE IN THE SAME ID STACK AS BeginPopup()
  void update_popup(char *popup_str_id) {
    if (is_open && !ImGui::IsPopupOpen(popup_str_id))
      ImGui::OpenPopup(popup_str_id);
    else
      is_open = false;
  }

public:
  void open_popup() { is_open = true; }

  toml::table save() override {
    throw std::runtime_error("Popup widgets should not be serialized!");
  };
};
