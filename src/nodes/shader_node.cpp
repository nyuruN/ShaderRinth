#include "nodes/shader_node.h"

#include "geometry.h"
#include "graph.h"
#include "shader.h"
#include <imgui_stdlib.h>
#include <imnodes.h>

int FragmentShaderNode::add_uniform_pin(RenderGraph &graph, DataType type, std::string name) {
  int pinid;
  graph.register_pin(id, type, &pinid);
  uniform_pins.push_back(UniformPin{
    pinid,
    type,
    identifier : name,
  });
  return pinid;
}
void FragmentShaderNode::render(RenderGraph &graph) {
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

  auto shader = this->shader.lock();
  if (ImGui::BeginCombo("##hidelabel", bool(shader) ? shader->get_name().c_str() : "")) {
    auto shaders = this->shaders.lock();

    assert(shaders && "Shader assets not initialized!");

    for (auto const &pair : *shaders) {
      ImGui::PushID(pair.first);
      bool is_selected = (shader) && (shader_id == pair.first);
      if (ImGui::Selectable(pair.second->get_name().c_str(), is_selected)) {
        this->shader_id = pair.first;
        this->shader = pair.second;
      }
      // Set the initial focus when opening the combo (scrolling + keyboard
      // navigation focus)
      if (is_selected)
        ImGui::SetItemDefaultFocus();
      ImGui::PopID();
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
void FragmentShaderNode::onEnter(RenderGraph &graph) {
  graph.register_pin(id, DataType::Texture2D, &this->output_pin);

  // If preconfigured using clone()
  if (!uniform_pins.empty()) {
    for (auto &pin : uniform_pins) {
      graph.register_pin(id, pin.type, &pin.pinid);
    }
  }

  onLoad(graph);
}
void FragmentShaderNode::onLoad(RenderGraph &graph) {
  glGenFramebuffers(1, &image_fbo);
  glGenTextures(1, &image_colorbuffer);

  glBindTexture(GL_TEXTURE_2D, image_colorbuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, graph.viewport_resolution.x, graph.viewport_resolution.y,
               0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
}
void FragmentShaderNode::onExit(RenderGraph &graph) {
  graph.delete_pin(output_pin);
  for (auto &pin : uniform_pins) {
    graph.delete_pin(pin.pinid);
  }
  glDeleteFramebuffers(1, &image_fbo);
  glDeleteTextures(1, &image_colorbuffer);
}
void FragmentShaderNode::run(RenderGraph &graph) {
  auto shader = this->shader.lock();

  if (!shader) {
    return graph.stop();
  }

  if (!shader->is_compiled()) {
    static bool should_error = true;
    if (!shader->compile(graph.graph_geometry)) {
      if (should_error) {
        spdlog::error(shader->get_log());
      }
      should_error = false; // Don't error next time
      return graph.stop();
    } else {
      should_error = true;
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
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, image_colorbuffer, 0);
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
