#pragma once

#include "graph.h"
#include "node.h"
#include <imnodes.h>

#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>

class ViewportNode : public Node {
private:
  int output_pin;

  float node_width = 120.0f;

public:
  int get_output_pin() { return output_pin; }
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Viewport");
    ImNodes::EndNodeTitleBar();
    {
      BEGIN_OUTPUT_PIN(output_pin, DataType::Vec2);
      ImGui::Indent(node_width - ImGui::CalcTextSize("Resolution").x);
      ImGui::Text("Resolution");
      END_OUTPUT_PIN();
    }
    ImGui::Dummy(ImVec2(node_width, 20.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Vec2, &output_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override {
    ImVec2 res = graph.viewport_resolution;
    graph.set_pin_data(output_pin, Data::Vec2({res.x, res.y}));
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin);
  }
};

// Type registration
#include <cereal/archives/json.hpp>
CEREAL_REGISTER_TYPE(ViewportNode)
