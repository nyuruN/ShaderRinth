#pragma once

#include <imgui.h>

#include <toml++/toml.hpp>

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
};

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
