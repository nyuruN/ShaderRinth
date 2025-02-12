#pragma once

#include "graph.h"
#include "nodes/output_node.h"
#include "widget.h"

#include <cereal/types/polymorphic.hpp>

/// TODO:
/// Add option to display the image without stretching
class ViewportWidget : public Widget {
private:
  std::shared_ptr<RenderGraph> viewgraph;
  std::string title;
  ImVec2 wsize = ImVec2(640, 480);

public:
  ViewportWidget(std::shared_ptr<RenderGraph> graph) { viewgraph = graph; }
  void render() override {
    ImGui::Begin("Viewport");
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
  static void load_and_construct(Archive &ar,
                                 cereal::construct<ViewportWidget> &construct) {
    std::shared_ptr<RenderGraph> viewgraph;
    ar(viewgraph);
    construct(viewgraph);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(viewgraph)); }
};

// Type registration
#include <cereal/archives/json.hpp>
CEREAL_REGISTER_TYPE(ViewportWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, ViewportWidget)
