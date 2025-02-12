#pragma once

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
  virtual void render() {};

  template <class Archive> void serialize(Archive &ar) {}
};
