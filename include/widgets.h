#pragma once

#include "editor.h"
#include "graph.h"
#include "nodes.h"
#include "utils.h"
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <zep.h>

// A stateful widget
class Widget {
public:
  // Runs on the first frame when loaded
  virtual void onStartup() {};
  // Runs on shutdown or otherwise destroyed
  virtual void onShutdown() {};
  // Runs every frame
  virtual void onUpdate() {};
  // Runs every frame if visible
  virtual void render() {};
  template <class Archive> void serialize(Archive &ar) {}
};

class EditorWidget : public Widget {
private:
  std::shared_ptr<Shader> shader;
  bool is_dirty = false;
  uint64_t last_update = 0;

public:
  EditorWidget(std::shared_ptr<Shader> shader) : shader(shader) {}
  template <class Archive>
  static void load_and_construct(Archive &ar,
                                 cereal::construct<EditorWidget> &construct) {
    std::shared_ptr<Shader> shader;
    ar(shader);
    construct(shader);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(shader)); }
  std::string get_buffer_text() {
    auto buffer = zep_get_editor().GetBuffers()[0];
    return buffer->GetBufferText(buffer->Begin(), buffer->End());
  }
  void onStartup() override {
    zep_init(Zep::NVec2f(1.0f, 1.0f));
    auto path = shader->get_path();
    zep_get_editor().InitWithText(path.filename().string(),
                                  shader->get_source());
    last_update = zep_get_editor().GetBuffers()[0]->GetUpdateCount();
  };
  void onShutdown() override { zep_destroy(); }
  void onUpdate() override {
    uint64_t new_update = zep_get_editor().GetBuffers()[0]->GetUpdateCount();
    if (new_update != last_update)
      this->is_dirty = true;
    last_update = new_update;

    if (is_dirty) {
      std::string text = get_buffer_text();
      shader->set_source(text);
      shader->should_recompile();
      is_dirty = false;
    }
  };
  void render() override {
    zep_show(Zep::NVec2i(0, 0), shader->get_name().c_str());
  }
};
/// TODO:
/// Add option to display the image without stretching
class ViewportWidget : public Widget {
private:
  std::shared_ptr<RenderGraph> viewgraph;
  ImVec2 wsize = ImVec2(640, 480);

public:
  ViewportWidget(std::shared_ptr<RenderGraph> graph) { viewgraph = graph; }
  template <class Archive>
  static void load_and_construct(Archive &ar,
                                 cereal::construct<ViewportWidget> &construct) {
    std::shared_ptr<RenderGraph> viewgraph;
    ar(viewgraph);
    construct(viewgraph);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(viewgraph)); }
  void onStartup() override {};
  void onShutdown() override {}
  void onUpdate() override {};
  void render() override {
    ImGui::Begin("Viewport");
    ImGui::BeginChild("ViewportRender");

    wsize = ImGui::GetWindowSize();

    viewgraph->clear_graph_data();
    viewgraph->set_resolution(wsize);
    viewgraph->evaluate();
    auto out = dynamic_cast<OutputNode *>(viewgraph->get_root_node());
    GLuint output = out->get_image();

    ImGui::Image((ImTextureID)output, wsize, ImVec2(0, 1), ImVec2(1, 0));

    ImGui::EndChild();
    ImGui::End();
  }
};
/// TODO:
/// Add custom colors to console output
class ConsoleWidget : public Widget {
private:
  bool sticky = false;

public:
  template <class Archive> void serialize(Archive &ar) { ar(sticky); }
  void onStartup() override {}
  void onShutdown() override {}
  void onUpdate() override {}
  void render() override {
    ImGui::Begin("Console");

    ImGui::TextWrapped("%s",
                       Global::instance().get_log_stream_string().c_str());

    float max_y = ImGui::GetScrollMaxY();
    float y = ImGui::GetScrollY();

    if (sticky)
      ImGui::SetScrollY(max_y);

    sticky = (y >= max_y);

    ImGui::End();
  }
};
class NodeEditorWidget : public Widget {
private:
  std::shared_ptr<RenderGraph> graph = nullptr;
  ImNodesEditorContext *context = nullptr;
  UndoContext history = UndoContext(50);

public:
  NodeEditorWidget(std::shared_ptr<RenderGraph> graph) : graph(graph) {}
  template <class Archive>
  static void
  load_and_construct(Archive &ar,
                     cereal::construct<NodeEditorWidget> &construct) {
    std::shared_ptr<RenderGraph> graph;
    ar(graph);
    construct(graph);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(graph)); }
  void onStartup() override {
    context = ImNodes::EditorContextCreate();
    ImNodes::EditorContextSet(context);
    graph->set_node_positions();
  };
  void onShutdown() override { ImNodes::EditorContextFree(context); }
  void onUpdate() override {
    int from_pin, to_pin;
    if (ImNodes::IsLinkCreated(&from_pin, &to_pin)) {
      graph->insert_edge(from_pin, to_pin);
      spdlog::info("Link created from {} to {}", from_pin, to_pin);
    }
  }
  void render() override {
    ImGui::Begin("Node Editor");
    ImNodes::EditorContextSet(context);
    ImNodes::BeginNodeEditor();

    Global::instance().set_undo_context(&history);
    graph.get()->render();

    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 5);
    if (ImGui::BeginPopup("Add Nodes")) {
      ImGui::Dummy(ImVec2(100, 0)); // Min width
      ImGui::Indent(5);
      if (ImGui::Selectable("Vec2"))
        graph->insert_node(std::make_shared<Vec2Node>());
      if (ImGui::Selectable("Float"))
        graph->insert_node(std::make_shared<FloatNode>());
      if (ImGui::Selectable("Fragment Shader"))
        graph->insert_node(std::make_shared<FragmentShaderNode>());
      if (ImGui::Selectable("Time"))
        graph->insert_node(std::make_shared<TimeNode>());
      ImGui::Spacing();
      ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    auto io = ImGui::GetIO();
    bool pressed = false;
    bool focused = ImGui::IsWindowFocused();
    if (focused) {
      pressed = isKeyJustPressed(ImGuiKey_Z);
      if (pressed && io.KeyCtrl && io.KeyShift)
        history.redo();
      else if (pressed && io.KeyCtrl)
        history.undo();
      pressed = isKeyJustPressed(ImGuiKey_A);
      if (pressed && io.KeyShift) {
        ImGui::OpenPopup("Add Nodes");
      }
    }

    ImNodes::EndNodeEditor();
    ImGui::End();

    if (focused) {
      if (isKeyJustPressed(ImGuiKey_X)) {
        int length = ImNodes::NumSelectedNodes();
        if (length > 0) {
          int nodeids[length];
          ImNodes::GetSelectedNodes(nodeids);
          for (int i = 0; i < length; i++) {
            graph->delete_node(nodeids[i]);
          }
        }
      }
    }
  }
};

// Type registration
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/xml.hpp>

CEREAL_REGISTER_TYPE(EditorWidget)
CEREAL_REGISTER_TYPE(ViewportWidget)
CEREAL_REGISTER_TYPE(ConsoleWidget)
CEREAL_REGISTER_TYPE(NodeEditorWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, EditorWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, ViewportWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, ConsoleWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, NodeEditorWidget)
