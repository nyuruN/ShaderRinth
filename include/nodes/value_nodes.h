#pragma once

#include "graph.h"
#include "node.h"
#include <imnodes.h>

#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>

class FloatNode : public Node {
  int output_pin;

  Data::Float prev_value = 0.0f, value = 0.0f;

  float node_width = 80.0f;

public:
  void render(RenderGraph &graph) override {
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
  void run(RenderGraph &graph) override {
    graph.set_pin_data(output_pin, (Data::Float)value);
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin, value);
  }
};

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
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Vec2");
    ImNodes::EndNodeTitleBar();
    {
      BEGIN_OUTPUT_PIN(output_pin, DataType::Vec2);
      ImGui::SetNextItemWidth(node_width);
      ImGui::InputFloat2("##hidelabel", value.data(), "%.1f",
                         ImGuiInputTextFlags_NoUndoRedo);
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
    graph.register_pin(id, DataType::Vec2, &output_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override {
    graph.set_pin_data(output_pin, (Data::Vec2)value);
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin, value);
  }
};

// Type registration
#include <cereal/archives/json.hpp>
CEREAL_REGISTER_TYPE(FloatNode)
CEREAL_REGISTER_TYPE(Vec2Node)
