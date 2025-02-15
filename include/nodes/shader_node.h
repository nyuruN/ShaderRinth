#pragma once

#include "geometry.h"
#include "graph.h"
#include "node.h"
#include "shader.h"
#include <GLFW/glfw3.h>
#include <imgui_stdlib.h>
#include <imnodes.h>

#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>

class FragmentShaderNode : public Node {
  struct UniformPin {
    int pinid;
    DataType type;
    std::string identifier;

    template <class Archive> void serialize(Archive &ar) { ar(pinid, type, identifier); }
  };

  std::vector<UniformPin> uniform_pins;
  int output_pin;
  AssetId<Shader> shader_id;

  std::shared_ptr<Shader> shader;
  GLuint image_fbo;
  GLuint image_colorbuffer;

  const float node_width = 240.0f;

public:
  void set_shader(std::shared_ptr<AssetManager> assets, AssetId<Shader> shader_id) {
    this->shader = assets->get_shader(shader_id);
    this->shader_id = shader_id;
  }
  int get_output_pin() { return output_pin; }
  int add_uniform_pin(RenderGraph &graph, DataType type, std::string name) {
    int pinid;
    graph.register_pin(id, type, &pinid);
    uniform_pins.push_back(UniformPin{
      pinid,
      type,
      identifier : name,
    });
    return pinid;
  }
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("FragmentShader");
    ImGui::SameLine();
    ImGui::Indent(node_width - ImGui::CalcTextSize(" + ").x);
    if (ImGui::Button(" + "))
      ImGui::OpenPopup("add_uniform_popup");
    ImNodes::EndNodeTitleBar();

    ImGui::Text("Source");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(node_width - ImGui::CalcTextSize("Source").x);

    if (ImGui::BeginCombo("##hidelabel", bool(shader) ? shader->get_name().c_str() : "")) {
      for (auto const &pair : *graph.shaders) {
        bool is_selected = (shader) && (shader->get_name() == pair.second->get_name());
        if (ImGui::Selectable(pair.second->get_name().c_str(), is_selected)) {
          shader_id = pair.first;
          shader = pair.second;
        }
        // Set the initial focus when opening the combo (scrolling + keyboard
        // navigation focus)
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    {
      BEGIN_OUTPUT_PIN(output_pin, DataType::Texture2D);
      ImGui::Indent(node_width - ImGui::CalcTextSize("Image").x);
      ImGui::Text("Image");
      END_OUTPUT_PIN();
    }

    if (ImGui::BeginPopup("add_uniform_popup")) { // Creation logic
      ImGui::SeparatorText("Uniforms");
      for (auto &type : Data::ALL) {
        if (ImGui::MenuItem(Data::type_name(type))) {
          add_uniform_pin(graph, type, fmt::format("u_{}", Data::type_name(type)));
        }
      }
      ImGui::EndPopup();
    }

    { // Render uniforms + delete logic
      std::vector<int> marked;
      int idx = 0;
      for (auto &pin : uniform_pins) {
        BEGIN_INPUT_PIN(pin.pinid, pin.type)

        ImGui::Text(Data::type_name(pin.type));
        ImGui::SameLine();

        ImGui::SetNextItemWidth(node_width - 20 - ImGui::CalcTextSize(" - ").x -
                                ImGui::CalcTextSize(Data::type_name(pin.type)).x);
        ImGui::InputText("##hidelabel", &pin.identifier);

        ImGui::SameLine();
        ImGui::Indent(node_width - ImGui::CalcTextSize(" - ").x);
        if (ImGui::Button(" - ")) {
          marked.push_back(idx);
        }

        END_INPUT_PIN();
        idx++;
      }
      for (auto &i : marked) {
        graph.delete_pin(uniform_pins[i].pinid);
        uniform_pins.erase(uniform_pins.begin() + i);
      }
    }

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Texture2D, &this->output_pin);

    // If preconfigured using clone()
    if (!uniform_pins.empty()) {
      for (auto &pin : uniform_pins) {
        graph.register_pin(id, pin.type, &pin.pinid);
      }
    }

    onLoad(graph);
  }
  void onLoad(RenderGraph &graph) override {
    glGenFramebuffers(1, &image_fbo);
    glGenTextures(1, &image_colorbuffer);

    glBindTexture(GL_TEXTURE_2D, image_colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, graph.viewport_resolution.x, graph.viewport_resolution.y,
                 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  void onExit(RenderGraph &graph) override {
    graph.delete_pin(output_pin);
    for (auto &pin : uniform_pins) {
      graph.delete_pin(pin.pinid);
    }
    glDeleteFramebuffers(1, &image_fbo);
    glDeleteTextures(1, &image_colorbuffer);
  }
  void run(RenderGraph &graph) override {
    if (!shader) {
      graph.stop();
      return;
    }

    if (!shader->is_compiled()) {
      static bool should_error = true;
      if (!shader->compile(graph.graph_geometry)) {
        // TODO: Likewise find a better way to delegate
        // error messages to the user -> interaction with
        // editor?
        if (should_error)
          spdlog::error(shader->get_log());
        should_error = false; // Don't error next time
        graph.stop();
        return;
      } else { // If compiled
        should_error = true;
        // TODO: Find a better way to display status
        // spdlog::info("Shader compiled");
      }
    }

    glBindTexture(GL_TEXTURE_2D, image_colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, graph.viewport_resolution.x, graph.viewport_resolution.y,
                 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (auto &pin : uniform_pins) {
      Data data = graph.get_pin_data(pin.pinid);
      if (data)
        shader->set_uniform(pin.identifier.c_str(), data);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, image_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, image_colorbuffer,
                           0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      graph.stop();
      spdlog::error("Framebuffer is not complete!");
      return;
    }

    glViewport(0, 0, graph.viewport_resolution.x, graph.viewport_resolution.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader->use();
    graph.graph_geometry->draw_geometry();
    shader->clear_textures();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    graph.set_pin_data(output_pin, (Data::Texture2D)image_colorbuffer);
  }

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
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin, uniform_pins, shader);
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
  static FragmentShaderNode load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    auto n = FragmentShaderNode();
    n.id = tbl["node_id"].value<int>().value();
    n.pos = Node::load_pos(*tbl["position"].as_table());
    n.output_pin = tbl["output_pin"].value<int>().value();
    n.shader_id = tbl["shader_id"].value<int>().value();
    n.shader = assets->get_shader(n.shader_id);

    for (auto &n_pin : *tbl["uniform_pins"].as_array()) {
      toml::table *t_pin = n_pin.as_table();

      int pin_id = (*t_pin)["pin_id"].value<int>().value();
      int type = (*t_pin)["type"].value<int>().value();
      std::string identifier = (*t_pin)["identifier"].value<std::string>().value();

      n.uniform_pins.push_back(UniformPin{
        pinid : pin_id,
        type : DataType(type),
        identifier : identifier,
      });
    }

    return n;
  }
};

// Type registration
#include <cereal/archives/json.hpp>
CEREAL_REGISTER_TYPE(FragmentShaderNode)
