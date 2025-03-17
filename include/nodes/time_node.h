#pragma once

#include "graph.h"
#include "imnodes.h"
#include "node.h"

class TimeNode : public Node {
  int output_pin;

  float node_width = 80.0f;

public:
  int get_output_pin() { return output_pin; }
  void render(RenderGraph &) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Time");
    ImNodes::EndNodeTitleBar();
    {
      BEGIN_OUTPUT_PIN(output_pin, DataType::Float);
      ImGui::Indent(node_width - ImGui::CalcTextSize("Float").x);
      ImGui::Text("Float");
      END_OUTPUT_PIN();
    }
    ImGui::Dummy(ImVec2(node_width, 10.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Float, &output_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override { graph.set_pin_data(output_pin, (Data::Float)graph.time); }

  std::shared_ptr<Node> clone() const override { return std::make_shared<TimeNode>(*this); }
  std::vector<int> layout() const override { return {output_pin}; }
  toml::table save() override {
    return toml::table{
        {"type", "TimeNode"},
        {"node_id", id},
        {"position", Node::save(pos)},
        {"output_pin", output_pin},
    };
  }
  static std::shared_ptr<Node> load(toml::table &tbl, std::shared_ptr<AssetManager>) {
    auto n = TimeNode();
    n.id = tbl["node_id"].value<int>().value();
    n.pos = Node::load_pos(*tbl["position"].as_table());
    n.output_pin = tbl["output_pin"].value<int>().value();
    return std::make_shared<TimeNode>(n);
  }
};

REGISTER_NODE_FACTORY(TimeNode)
