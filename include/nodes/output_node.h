#pragma once

#include "graph.h"
#include "node.h"
#include <imnodes.h>

#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>

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
    ImNodes::EndNodeTitleBar();
    {
      BEGIN_INPUT_PIN(input_pin, DataType::Texture2D);
      ImGui::Text("Image");
      END_INPUT_PIN();
    }
    ImGui::Dummy(ImVec2(node_width, 20.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Texture2D, &input_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(input_pin); }
  void run(RenderGraph &graph) override {
    static bool logged = false;
    Data data = graph.get_pin_data(input_pin);
    try {
      out_texture = data.get<Data::Texture2D>();
      logged = false;
    } catch (std::bad_any_cast) {
      if (!logged) {
        spdlog::error("Nothing is connected!");
        logged = true;
      }
    }
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(input_pin, out_texture);
  }
};

// Type registration
#include <cereal/archives/json.hpp>
CEREAL_REGISTER_TYPE(OutputNode)
