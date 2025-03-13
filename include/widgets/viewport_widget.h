#pragma once

#include "assets.h"
#include "widget.h"
#include <fmt/format.h>

// Forward declares
struct RenderGraph;

/// A viewport to evaluate a RenderGraph in real-time
class ViewportWidget : public Widget {
private:
  AssetId<RenderGraph> graph_id;

  std::shared_ptr<RenderGraph> viewgraph;
  std::string title;
  ImVec2 wsize = ImVec2(640, 480);
  double last_time = 0.0f;

  bool paused = false;

public:
  ViewportWidget(int id, std::shared_ptr<AssetManager> assets, AssetId<RenderGraph> graph_id) {
    this->id = id;
    title = fmt::format("Viewport##{}", id);
    viewgraph = assets->getRenderGraph(graph_id).value();
    this->graph_id = graph_id;
  }
  void render(bool *p_open) override;

  toml::table save() override {
    return toml::table{
        {"type", "ViewportWidget"},
        {"widget_id", id},
        {"graph_id", graph_id},
    };
  }
  static std::shared_ptr<Widget> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    int id = tbl["widget_id"].value<int>().value();
    AssetId<RenderGraph> graph_id = tbl["graph_id"].value<int>().value();
    auto w = ViewportWidget(id, assets, graph_id);
    return std::make_shared<ViewportWidget>(w);
  }
};

REGISTER_WIDGET_FACTORY(ViewportWidget);
