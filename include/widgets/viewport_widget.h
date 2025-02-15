#pragma once

#include "graph.h"
#include "nodes/output_node.h"
#include "widget.h"

#include <cereal/types/polymorphic.hpp>

/// TODO:
/// Add option to display the image without stretching
class ViewportWidget : public Widget {
private:
  AssetId<RenderGraph> graph_id;

  std::shared_ptr<RenderGraph> viewgraph;
  std::string title;
  ImVec2 wsize = ImVec2(640, 480);

public:
  ViewportWidget(int id, std::shared_ptr<AssetManager> assets, AssetId<RenderGraph> graph_id) {
    this->id = id;
    title = fmt::format("Viewport##{}", id);
    viewgraph = assets->get_graph(graph_id);
    this->graph_id = graph_id;
  }
  void render(bool *p_open) override {
    ImGui::SetNextWindowSize({400, 400}, ImGuiCond_FirstUseEver);
    ImGui::Begin(title.c_str(), p_open);
    ImGui::BeginChild("ViewportRender");

    wsize = ImGui::GetWindowSize();

    viewgraph->clear_graph_data();
    viewgraph->set_resolution(wsize);
    viewgraph->evaluate();

    auto out = dynamic_cast<OutputNode *>(viewgraph->get_root_node());
    GLuint output = out->get_image();

    ImGui::Image((ImTextureID)output, wsize, ImVec2(0, 1), ImVec2(1, 0));

    ImGui::EndChild();
    ImGui::End();
  }

  template <class Archive>
  static void load_and_construct(Archive &ar, cereal::construct<ViewportWidget> &construct) {
    std::shared_ptr<RenderGraph> viewgraph;
    int id;
    ar(id, viewgraph);
    construct(id, viewgraph);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(id), VP(viewgraph)); }
  toml::table save() override {
    return toml::table{
        {"type", "ViewportWidget"},
        {"widget_id", id},
        {"graph_id", graph_id},
    };
  }
  static ViewportWidget load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    int id = tbl["widget_id"].value<int>().value();
    AssetId<RenderGraph> graph_id = tbl["graph_id"].value<int>().value();
    auto w = ViewportWidget(id, assets, graph_id);
    return w;
  }
};

// Type registration
#include <cereal/archives/json.hpp>
// CEREAL_REGISTER_TYPE(ViewportWidget)
// CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, ViewportWidget)
