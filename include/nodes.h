#pragma once

#include "graph.h"
#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>
#include <imgui_stdlib.h>

#define BEGIN_INPUT_PIN(id, type)                                              \
  ImNodes::PushColorStyle(ImNodesCol_Pin, Data::COLORS[type]);                 \
  ImNodes::PushColorStyle(ImNodesCol_PinHovered, Data::COLORS_HOVER[type]);    \
  ImNodes::BeginInputAttribute(id);                                            \
  ImNodes::PopColorStyle();                                                    \
  ImNodes::PopColorStyle();

#define END_INPUT_PIN() ImNodes::EndInputAttribute();

#define BEGIN_OUTPUT_PIN(id, type)                                             \
  ImNodes::PushColorStyle(ImNodesCol_Pin, Data::COLORS[type]);                 \
  ImNodes::PushColorStyle(ImNodesCol_PinHovered, Data::COLORS_HOVER[type]);    \
  ImNodes::BeginOutputAttribute(id);                                           \
  ImNodes::PopColorStyle();                                                    \
  ImNodes::PopColorStyle();

#define END_OUTPUT_PIN() ImNodes::EndOutputAttribute();

// ============
// Custom Nodes
// ============

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
  void run(RenderGraph &graph) override {
    graph.set_pin_data(output_pin, (Data::Float)glfwGetTime());
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin);
  }
};
class ViewportNode : public Node {
private:
  int output_pin;

  float node_width = 120.0f;

public:
  int get_output_pin() { return output_pin; }
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Viewport");
    ImNodes::EndNodeTitleBar();
    {
      BEGIN_OUTPUT_PIN(output_pin, DataType::Vec2);
      ImGui::Indent(node_width - ImGui::CalcTextSize("Resolution").x);
      ImGui::Text("Resolution");
      END_OUTPUT_PIN();
    }
    ImGui::Dummy(ImVec2(node_width, 20.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Vec2, &output_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override {
    ImVec2 res = graph.viewport_resolution;
    graph.set_pin_data(output_pin, Data::Vec2({res.x, res.y}));
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
      // ImNodes::BeginOutputAttribute(output_pin);
      BEGIN_OUTPUT_PIN(output_pin, DataType::Float);
      ImGui::SetNextItemWidth(node_width);
      ImGui::InputFloat("Float", &value, 0, 0, "%.2f");
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        getUndoContext()->do_action(
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
        getUndoContext()->do_action(
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
  std::vector<GLuint> bound_textures = {};

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
      BEGIN_OUTPUT_PIN(output_pin, DataType::Texture2D);
      ImGui::Indent(node_width - ImGui::CalcTextSize("Image").x);
      ImGui::Text("Image");
      END_OUTPUT_PIN();
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
        BEGIN_INPUT_PIN(pin.pinid, pin.type)

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
    bound_textures.clear();

    graph.set_pin_data(output_pin, (Data::Texture2D)image_colorbuffer);
  }
  unsigned int get_next_texture_unit(GLuint texture) {
    unsigned int unit = bound_textures.size();
    bound_textures.push_back(texture);
    return GL_TEXTURE0 + unit;
  }
  void set_uniform(GLuint loc, std::string identifier, Data data) {
    // clang-format off
    switch (data.type) {
    case DataType::Int: {
      auto val = data.get<Data::Int>(); glUniform1iv(loc, 1, &val); break;
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
      int unit = get_next_texture_unit(texture);
      glActiveTexture(unit);
      glBindTexture(GL_TEXTURE_2D, texture);
      glUniform1i(loc, bound_textures.size() - 1);
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
class Texture2DNode : public Node {
private:
  int output_pin;

  std::shared_ptr<Texture> texture;

  float node_width = 120.0f;

public:
  void set_texture(std::shared_ptr<Texture> texture) {
    this->texture = texture;
  }
  int get_output_pin() { return output_pin; }
  void render_combobox(RenderGraph &graph) {
    ImGui::SetNextItemWidth(node_width);
    if (ImGui::BeginCombo("##hidelabel",
                          bool(texture) ? texture->get_name().c_str() : "")) {
      for (auto const &pair : *graph.textures) {
        bool is_selected =
            (texture) && (texture->get_name() == pair.second->get_name());
        if (ImGui::Selectable(pair.first.c_str(), is_selected))
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
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin, texture);
  }
};

// Type registration
#include <cereal/archives/json.hpp>

CEREAL_REGISTER_TYPE(OutputNode)
CEREAL_REGISTER_TYPE(TimeNode)
CEREAL_REGISTER_TYPE(ViewportNode)
CEREAL_REGISTER_TYPE(FloatNode)
CEREAL_REGISTER_TYPE(Vec2Node)
CEREAL_REGISTER_TYPE(FragmentShaderNode)
CEREAL_REGISTER_TYPE(Texture2DNode)
