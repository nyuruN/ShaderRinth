#pragma once

#include "graph.h"
#include "imnodes.h"
#include "node.h"
#include "texture.h"

class Texture2DNode : public Node {
private:
  int output_pin;

  AssetId<Texture> texture_id = 0;
  std::weak_ptr<Texture> texture;
  std::weak_ptr<Assets<Texture>> textures;

  float node_width = 120.0f;

public:
  Texture2DNode(std::shared_ptr<AssetManager> assets) {
    this->textures = assets->getTextureCollection();
  }
  int get_output_pin() { return output_pin; }
  void render_combobox(RenderGraph &graph) {
    ImGui::SetNextItemWidth(node_width);
    auto texture = this->texture.lock();
    if (ImGui::BeginCombo("##hidelabel", bool(texture) ? texture->get_name().c_str() : "")) {
      auto textures = this->textures.lock();

      assert(textures && "Texture assets not initialized!");

      for (auto const &pair : *textures) {
        ImGui::PushID(pair.first);
        bool is_selected = (texture) && (texture_id == pair.first);
        if (ImGui::Selectable(pair.second->get_name().c_str(), is_selected)) {
          this->texture_id = pair.first;
          this->texture = pair.second;
        }
        // Set the initial focus when opening the combo (scrolling + keyboard
        // navigation focus)
        if (is_selected)
          ImGui::SetItemDefaultFocus();
        ImGui::PopID();
      }
      ImGui::EndCombo();
    }
  }
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Texture Node");
    ImNodes::EndNodeTitleBar();

    if (texture.lock()) {
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
    if (auto texture = this->texture.lock())
      graph.set_pin_data(output_pin, (Data::Texture2D)texture->get_texture());
  }

  std::shared_ptr<Node> clone() const override { return std::make_shared<Texture2DNode>(*this); }
  std::vector<int> layout() const override { return {output_pin}; }
  toml::table save() override {
    return toml::table{
        {"type", "Texture2DNode"},     //
        {"node_id", id},               //
        {"position", Node::save(pos)}, //
        {"output_pin", output_pin},    //
        {"texture_id", texture_id},    //
    };
  }
  static std::shared_ptr<Node> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    auto n = Texture2DNode(assets);
    n.id = tbl["node_id"].value<int>().value();
    n.pos = Node::load_pos(*tbl["position"].as_table());
    n.texture_id = tbl["texture_id"].value<int>().value();
    n.output_pin = tbl["output_pin"].value<int>().value();
    if (n.texture_id)
      n.texture = assets->getTexture(n.texture_id).value();
    return std::make_shared<Texture2DNode>(n);
  }
};

REGISTER_NODE_FACTORY(Texture2DNode)
