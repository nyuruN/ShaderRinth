#pragma once

#include "widget.h"
#include <filesystem>

// Forward declares
struct RenderGraph;

/// Popup widget, should not be serialized
class ExportImagePopup : public PopupWidget {
private:
  enum Format { PNG, JPEG };

  std::shared_ptr<RenderGraph> graph = nullptr;

  std::string export_path = "";
  int resolution[2] = {1920, 1080};
  int format = PNG;
  int quality = 90;

  float widget_width = 480;

public:
  ExportImagePopup() {}
  ExportImagePopup(std::shared_ptr<RenderGraph> graph) { this->graph = graph; }
  void set_graph(std::shared_ptr<RenderGraph> graph) { this->graph = graph; }
  void set_extension(const std::filesystem::path &ext) {
    std::filesystem::path path(export_path);
    path.replace_extension(ext);
    export_path = path.string();
  }
  // Saves a render with specified settings
  void export_image();
  virtual void onStartup() override {
    // Default image path
    auto pwd = std::filesystem::current_path();
    switch (format) {
    case PNG:
      export_path = pwd / "image.png";
      break;
    case JPEG:
      export_path = pwd / "image.jpg";
      break;
    }
  }
  void render(bool *p_open = NULL) override;
  virtual void onShutdown() override { graph.reset(); }
};
