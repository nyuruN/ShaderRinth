#pragma once

#include "graph.h"
#include "imnodes.h"
#include "node.h"
#ifndef __gl_h_
#include "glad/glad.h"
#endif
#include <spdlog/spdlog.h>

class OutputNode : public Node {
private:
  int input_pin;
  GLuint out_texture;

  float node_width = 80.0f;

public:
  int get_input_pin() { return input_pin; }
  GLuint get_image() { return out_texture; }
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Output");
    ImGui::SameLine();
    ImGui::Indent(node_width - 7.5);

    bool selected = graph.get_root_node_id() == id;
    bool clicked = ImGui::Checkbox("##hidelabel", &selected);
    if (clicked && !selected) // If unchecked
      graph.set_root_node(-1);
    else if (clicked)
      graph.set_root_node(id);

    ImNodes::EndNodeTitleBar();
    {
      BEGIN_INPUT_PIN(input_pin, DataType::Texture2D);
      ImGui::Text("Image");
      END_INPUT_PIN();
    }
    ImGui::Dummy(ImVec2(node_width, 15.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Texture2D, &input_pin);
    graph.set_root_node(id);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(input_pin); }
  void run(RenderGraph &graph) override {
    static bool logged = false;
    Data data = graph.get_pin_data(input_pin);
    try {
      out_texture = data.get<Data::Texture2D>();
      logged = false;
    } catch (std::bad_any_cast &) {
      if (!logged) {
        spdlog::error("Nothing is connected!");
        logged = true;
      }
    }
  }

  std::shared_ptr<Node> clone() const override { return std::make_shared<OutputNode>(*this); }
  std::vector<int> layout() const override { return {input_pin}; }
  toml::table save() override {
    return toml::table{
        {"type", "OutputNode"},        //
        {"node_id", id},               //
        {"position", Node::save(pos)}, //
        {"input_pin", input_pin},      //
        {"out_texture", out_texture},  //
    };
  }
  static std::shared_ptr<Node> load(toml::table &tbl, std::shared_ptr<AssetManager>) {
    auto n = OutputNode();
    n.id = tbl["node_id"].value<int>().value();
    n.pos = Node::load_pos(*tbl["position"].as_table());
    n.input_pin = tbl["input_pin"].value<int>().value();
    n.out_texture = tbl["out_texture"].value<int>().value();
    return std::make_shared<OutputNode>(n);
  }
};

REGISTER_NODE_FACTORY(OutputNode)
