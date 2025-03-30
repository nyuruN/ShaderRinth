#pragma once

#include "assets.h"
#include "node.h"
#ifndef __gl_h_
#include "glad/gl.h"
#endif

class FragmentShaderNode : public Node {
  struct UniformPin {
    int pinid;
    DataType type;
    std::string identifier;
  };

  std::vector<UniformPin> uniform_pins;
  AssetId<Shader> shader_id;
  int output_pin;

  std::weak_ptr<Assets<Shader>> shaders;
  std::weak_ptr<Shader> shader;
  GLuint image_fbo;
  GLuint image_colorbuffer;

  const float node_width = 240.0f;

public:
  FragmentShaderNode(std::shared_ptr<AssetManager> assets)
      : shaders(assets->getShaderCollection()) {}
  // Programmatically set a shader
  void set_shader(AssetId<Shader> shader_id) {
    this->shader = shaders.lock()->at(shader_id);
    this->shader_id = shader_id;
  }
  int get_output_pin() { return output_pin; }
  // Sets up a new uniform
  int add_uniform_pin(RenderGraph &graph, DataType type, std::string name);
  // Renders the node
  void render(RenderGraph &graph) override;
  // Sets up existing uniform and output pins
  void onEnter(RenderGraph &graph) override;
  // Sets up necessary OpenGL resources on load
  void onLoad(RenderGraph &graph) override;
  // Cleans up OpenGL resources and deletes registered pins
  void onExit(RenderGraph &graph) override;
  // Executes the shader
  void run(RenderGraph &graph) override;

  // OnEnter() will overwrite registered pins
  std::shared_ptr<Node> clone() const override {
    return std::make_shared<FragmentShaderNode>(*this);
  }
  std::vector<int> layout() const override {
    std::vector<int> l = {output_pin};
    l.reserve(uniform_pins.size() + 1);
    for (auto &pin : uniform_pins)
      l.push_back(pin.pinid);
    return l;
  }
  toml::table save() override {
    toml::array uniform_pins;

    for (auto &pin : this->uniform_pins) {
      toml::table t{
          {"pin_id", pin.pinid},
          {"identifier", pin.identifier},
          {"type", pin.type},
      };
      t.is_inline(true);
      uniform_pins.push_back(t);
    }

    return toml::table{
        {"type", "FragmentShaderNode"}, //
        {"node_id", id},                //
        {"position", Node::save(pos)},  //
        {"output_pin", output_pin},     //
        {"uniform_pins", uniform_pins}, //
        {"shader_id", shader_id},       //
    };
  }
  static std::shared_ptr<Node> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    auto n = FragmentShaderNode(assets);
    n.id = tbl["node_id"].value<int>().value();
    n.pos = Node::load_pos(*tbl["position"].as_table());
    n.output_pin = tbl["output_pin"].value<int>().value();
    n.shader_id = tbl["shader_id"].value<int>().value();
    n.shader = assets->getShader(n.shader_id).value();

    if (!tbl["uniform_pins"].is_array_of_tables())
      throw std::bad_optional_access();
    for (auto &n_pin : *tbl["uniform_pins"].as_array()) {
      toml::table *t_pin = n_pin.as_table();

      int pin_id = (*t_pin)["pin_id"].value<int>().value();
      int type = (*t_pin)["type"].value<int>().value();
      std::string identifier = (*t_pin)["identifier"].value<std::string>().value();

      n.uniform_pins.push_back(UniformPin{
          .pinid = pin_id,
          .type = DataType(type),
          .identifier = identifier,
      });
    }

    return std::make_shared<FragmentShaderNode>(n);
  }
};

REGISTER_NODE_FACTORY(FragmentShaderNode)
