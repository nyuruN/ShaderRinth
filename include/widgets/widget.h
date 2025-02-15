#pragma once

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
  template <class Archive> void serialize(Archive &ar) {}
};
