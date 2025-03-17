#pragma once

#include "graph.h"
#include "imnodes.h"
#include "node.h"
#include "utils.h"

class FloatNode : public Node {
  int output_pin;

  Data::Float prev_value = 0.0f, value = 0.0f;

  float node_width = 80.0f;

public:
  void render(RenderGraph &) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Float");
    ImNodes::EndNodeTitleBar();
    {
      // ImNodes::BeginOutputAttribute(output_pin);
      BEGIN_OUTPUT_PIN(output_pin, DataType::Float);
      ImGui::SetNextItemWidth(node_width);
      ImGui::InputFloat("Float", &value, 0, 0, "%.2f");
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        Global::getUndoContext()->do_action(
            {[this, newvalue = value] { prev_value = value = newvalue; },
             [this, oldvalue = prev_value] { prev_value = value = oldvalue; }});
        prev_value = value;
      }
      END_OUTPUT_PIN();
    }
    ImGui::Dummy(ImVec2(node_width, 20));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Float, &output_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override { graph.set_pin_data(output_pin, (Data::Float)value); }

  std::shared_ptr<Node> clone() const override { return std::make_shared<FloatNode>(*this); }
  std::vector<int> layout() const override { return {output_pin}; }
  toml::table save() override {
    return toml::table{
        {"type", "FloatNode"},         //
        {"node_id", id},               //
        {"position", Node::save(pos)}, //
        {"output_pin", output_pin},    //
        {"value", value},              //
    };
  }
  static std::shared_ptr<Node> load(toml::table &tbl, std::shared_ptr<AssetManager>) {
    auto n = FloatNode();
    n.id = tbl["node_id"].value<int>().value();
    n.pos = Node::load_pos(*tbl["position"].as_table());
    n.output_pin = tbl["output_pin"].value<int>().value();
    n.value = tbl["value"].value<float>().value();
    return std::make_shared<FloatNode>(n);
  }
};

REGISTER_NODE_FACTORY(FloatNode)

class Vec2Node : public Node {
  int output_pin;

  Data::Vec2 prev_value = {0}, value = {0};

  float node_width = 120.0f;

public:
  void set_value(float x, float y) {
    value[0] = x;
    value[1] = y;
  }
  int get_output_pin() { return output_pin; }
  void render(RenderGraph &) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Vec2");
    ImNodes::EndNodeTitleBar();
    {
      BEGIN_OUTPUT_PIN(output_pin, DataType::Vec2);
      ImGui::SetNextItemWidth(node_width);
      ImGui::InputFloat2("##hidelabel", value.data(), "%.1f", ImGuiInputTextFlags_NoUndoRedo);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        Global::getUndoContext()->do_action(
            {[this, newvalue = value] { prev_value = value = newvalue; },
             [this, oldvalue = prev_value] { prev_value = value = oldvalue; }});
        prev_value = value;
      }
      END_OUTPUT_PIN();
    }
    ImGui::Dummy(ImVec2(node_width, 20));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override { graph.register_pin(id, DataType::Vec2, &output_pin); }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override { graph.set_pin_data(output_pin, (Data::Vec2)value); }

  std::shared_ptr<Node> clone() const override { return std::make_shared<Vec2Node>(*this); }
  std::vector<int> layout() const override { return {output_pin}; }
  toml::table save() override {
    return toml::table{
        {"type", "Vec2Node"},          //
        {"node_id", id},               //
        {"position", Node::save(pos)}, //
        {"output_pin", output_pin},    //
        {"value", Node::save(value)},  //
    };
  }
  static std::shared_ptr<Node> load(toml::table &tbl, std::shared_ptr<AssetManager>) {
    auto n = Vec2Node();
    n.id = tbl["node_id"].value<int>().value();
    n.pos = Node::load_pos(*tbl["position"].as_table());
    n.output_pin = tbl["output_pin"].value<int>().value();
    n.value = Node::load_pos(*tbl["value"].as_table());
    return std::make_shared<Vec2Node>(n);
  }
};

REGISTER_NODE_FACTORY(Vec2Node)
