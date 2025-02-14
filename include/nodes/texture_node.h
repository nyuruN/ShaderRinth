#pragma once

#include "graph.h"
#include "node.h"
#include "texture.h"
#include <imnodes.h>

#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>

class Texture2DNode : public Node {
private:
  int output_pin;

  std::shared_ptr<Texture> texture;

  float node_width = 120.0f;

public:
  void set_texture(std::shared_ptr<Texture> texture) { this->texture = texture; }
  int get_output_pin() { return output_pin; }
  void render_combobox(RenderGraph &graph) {
    ImGui::SetNextItemWidth(node_width);
    if (ImGui::BeginCombo("##hidelabel", bool(texture) ? texture->get_name().c_str() : "")) {
      for (auto const &pair : *graph.textures) {
        bool is_selected = (texture) && (texture->get_name() == pair.second->get_name());
        if (ImGui::Selectable(pair.second->get_name().c_str(), is_selected))
          set_texture(pair.second);
        // Set the initial focus when opening the combo (scrolling + keyboard
        // navigation focus)
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
  }
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Texture Node");
    ImNodes::EndNodeTitleBar();

    if (texture) {
      BEGIN_OUTPUT_PIN(output_pin, DataType::Texture2D);
      render_combobox(graph);
      END_OUTPUT_PIN();
    } else {
      render_combobox(graph);
    }

    ImGui::Dummy(ImVec2(node_width, 20));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Texture2D, &output_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override {
    graph.set_pin_data(output_pin, (Data::Texture2D)texture->get_texture());
  }
  std::shared_ptr<Node> clone() const override { return std::make_shared<Texture2DNode>(*this); }
  std::vector<int> layout() const override { return {output_pin}; }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin, texture);
  }
};

// Type registration
#include <cereal/archives/json.hpp>
CEREAL_REGISTER_TYPE(Texture2DNode)
