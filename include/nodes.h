#pragma once

#include "graph.h"
#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>
#include <imgui_stdlib.h>

// ============
// Custom Nodes
// ============

class OutputNode : public Node {
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
      ImNodes::BeginInputAttribute(input_pin);
      ImGui::Text("Image");
      ImNodes::EndInputAttribute();
    }
    ImGui::Dummy(ImVec2(node_width, 20.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Texture2D, &input_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(input_pin); }
  void run(RenderGraph &graph) override {
    Data data = graph.get_pin_data(input_pin);
    try {
      out_texture = data.get<GLuint>();
    } catch (std::bad_any_cast) {
      spdlog::error("Nothing is connected!");
    }
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(input_pin, out_texture);
  }
};
class TimeNode : public Node {
  int output_pin;

  float node_width = 80.0f;

public:
  int get_output_pin() { return output_pin; }
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Time");
    ImNodes::EndNodeTitleBar();
    {
      ImNodes::BeginOutputAttribute(output_pin);
      ImGui::Indent(node_width - ImGui::CalcTextSize("Float").x);
      ImGui::Text("Float");
      ImNodes::EndOutputAttribute();
    }
    ImGui::Dummy(ImVec2(node_width, 10.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Float, &output_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override {
    graph.set_pin_data(output_pin, (Data::Float)glfwGetTime());
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin);
  }
};
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
      ImNodes::BeginOutputAttribute(output_pin);
      ImGui::SetNextItemWidth(node_width);
      ImGui::InputFloat("Float", &value, 0, 0, "%.2f");
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        getUndoContext()->do_action(
            {[this, newvalue = value] { prev_value = value = newvalue; },
             [this, oldvalue = prev_value] { prev_value = value = oldvalue; }});
        prev_value = value;
      }
      ImNodes::EndOutputAttribute();
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
      ImNodes::BeginOutputAttribute(output_pin);
      ImGui::SetNextItemWidth(node_width);
      ImGui::InputFloat2("##hidelabel", value.data(), "%.1f",
                         ImGuiInputTextFlags_NoUndoRedo);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        getUndoContext()->do_action(
            {[this, newvalue = value] { prev_value = value = newvalue; },
             [this, oldvalue = prev_value] { prev_value = value = oldvalue; }});
        prev_value = value;
      }
      ImNodes::EndOutputAttribute();
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
class FragmentShaderNode : public Node {
  struct UniformPin {
    int pinid;
    DataType type;
    std::string identifier;

    template <class Archive> void serialize(Archive &ar) {
      ar(pinid, type, identifier);
    }
  };

  int output_pin;
  std::vector<UniformPin> uniform_pins;
  std::shared_ptr<Shader> shader;

  GLuint image_fbo;
  GLuint image_colorbuffer;
  GLuint bound_textures = 0;

  const float node_width = 240.0f;

public:
  void set_shader(std::shared_ptr<Shader> shader) { this->shader = shader; }
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

    if (ImGui::BeginCombo("##hidelabel",
                          bool(shader) ? shader->get_name().c_str() : "")) {
      for (auto const &pair : *graph.shaders) {
        bool is_selected =
            (shader) && (shader->get_name() == pair.second->get_name());
        if (ImGui::Selectable(pair.first.c_str(), is_selected))
          set_shader(pair.second);
        // Set the initial focus when opening the combo (scrolling + keyboard
        // navigation focus)
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    {
      ImNodes::BeginOutputAttribute(output_pin);
      ImGui::Indent(node_width - ImGui::CalcTextSize("Image").x);
      ImGui::Text("Image");
      ImNodes::EndOutputAttribute();
    }

    if (ImGui::BeginPopup("add_uniform_popup")) { // Creation logic
      ImGui::SeparatorText("Uniforms");
      for (auto &type : Data::ALL) {
        if (ImGui::MenuItem(Data::type_name(type))) {
          add_uniform_pin(graph, type,
                          fmt::format("u_{}", Data::type_name(type)));
        }
      }
      ImGui::EndPopup();
    }

    { // Render uniforms + delete logic
      std::vector<int> marked;
      int idx = 0;
      for (auto &pin : uniform_pins) {
        ImNodes::BeginInputAttribute(pin.pinid, (pin.type == DataType::Float)
                                                    ? ImNodesPinShape_Quad
                                                    : ImNodesPinShape_Circle);
        ImGui::Text(Data::type_name(pin.type));
        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            node_width - 20 - ImGui::CalcTextSize(" - ").x -
            ImGui::CalcTextSize(Data::type_name(pin.type)).x);
        ImGui::InputText("##hidelabel", &pin.identifier);

        ImGui::SameLine();
        ImGui::Indent(node_width - ImGui::CalcTextSize(" - ").x);
        if (ImGui::Button(" - ")) {
          marked.push_back(idx);
        }
        ImNodes::EndInputAttribute();
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
    onLoad(graph);
  }
  void onLoad(RenderGraph &graph) override {
    glGenFramebuffers(1, &image_fbo);
    glGenTextures(1, &image_colorbuffer);

    glBindTexture(GL_TEXTURE_2D, image_colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, graph.viewport_resolution.x,
                 graph.viewport_resolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 NULL);
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
        if (should_error)
          spdlog::error(shader->get_log());
        should_error = false; // Don't error next time
        graph.stop();
        return;
      } else { // If compiled
        should_error = true;
        spdlog::info("Shader compiled");
      }
    }

    glBindTexture(GL_TEXTURE_2D, image_colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, graph.viewport_resolution.x,
                 graph.viewport_resolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (auto &pin : uniform_pins) {
      Data data = graph.get_pin_data(pin.pinid);
      if (data)
        set_uniform(shader->get_uniform_loc(pin.identifier.data()),
                    pin.identifier, data);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, image_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           image_colorbuffer, 0);
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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    bound_textures = 0;

    graph.set_pin_data(output_pin, (Data::Texture2D)image_colorbuffer);
  }
  unsigned int get_next_texture_unit() {
    unsigned int unit = bound_textures;
    bound_textures++;
    return GL_TEXTURE0 + unit;
  }
  void set_uniform(GLuint loc, std::string identifier, Data data) {
    // clang-format off
    switch (data.type) {
    case DataType::Int: {
      auto val = data.get<int>(); glUniform1iv(loc, 1, &val); break;
    };
    case DataType::IVec2: {
      auto val = data.get<Data::IVec2>().data(); glUniform2iv(loc, 1, val); break;
    };
    case DataType::IVec3: {
      auto val = data.get<Data::IVec3>().data(); glUniform3iv(loc, 1, val); break;
    };
    case DataType::IVec4: {
      auto val = data.get<Data::IVec4>().data(); glUniform4iv(loc, 1, val); break;
    };
    case DataType::UInt: {
      auto val = data.get<Data::UInt>(); glUniform1uiv(loc, 1, &val); break;
    };
    case DataType::UVec2: {
      auto val = data.get<Data::UVec2>().data(); glUniform2uiv(loc, 1, val); break;
    };
    case DataType::UVec3: {
      auto val = data.get<Data::UVec3>().data(); glUniform3uiv(loc, 1, val); break;
    };
    case DataType::UVec4: {
      auto val = data.get<Data::UVec4>().data(); glUniform4uiv(loc, 1, val); break;
    };
    case DataType::Float: {
      auto val = data.get<Data::Float>(); glUniform1fv(loc, 1, &val); break;
    };
    case DataType::Vec2: {
      auto val = data.get<Data::Vec2>().data(); glUniform2fv(loc, 1, val); break;
    };
    case DataType::Vec3: {
      auto val = data.get<Data::Vec3>().data(); glUniform3fv(loc, 1, val); break;
    };
    case DataType::Vec4: {
      auto val = data.get<Data::Vec4>().data(); glUniform4fv(loc, 1, val); break;
    };
    case DataType::Texture2D: {
      GLuint texture = data.get<Data::Texture2D>();
      int unit = get_next_texture_unit();
      glActiveTexture(unit);
      glBindTexture(GL_TEXTURE_2D, texture);
      glUniform1i(loc, texture);
      break;
    };
    }
    // clang-format on
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin, uniform_pins, shader);
  }
};

// Type registration
#include <cereal/archives/json.hpp>

CEREAL_REGISTER_TYPE(OutputNode)
CEREAL_REGISTER_TYPE(TimeNode)
CEREAL_REGISTER_TYPE(FloatNode)
CEREAL_REGISTER_TYPE(Vec2Node)
CEREAL_REGISTER_TYPE(FragmentShaderNode)
