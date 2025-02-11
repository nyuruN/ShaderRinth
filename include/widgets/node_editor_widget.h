#pragma once

#include "graph.h"
#include "nodes.h"
#include "widget.h"
#include <imnodes.h>

#include <cereal/types/polymorphic.hpp>

class NodeEditorWidget : public Widget {
private:
  std::shared_ptr<RenderGraph> graph = nullptr;
  ImNodesEditorContext *context = ImNodes::EditorContextCreate();
  UndoContext history = UndoContext(50);

  // Start with a default type
  DataType current_link_type = DataType(0);
  bool add_nodes = false;
  bool focused = false;

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
  void onStartup() override { graph->set_node_positions(context); };
  void onShutdown() override { ImNodes::EditorContextFree(context); }
  void onUpdate() override {
    auto io = ImGui::GetIO();

    { // ImNodes events
      int from_pin, to_pin;
      if (ImNodes::IsLinkCreated(&from_pin, &to_pin)) {
        if (graph->insert_edge(from_pin, to_pin) != -1)
          spdlog::info("Link created from {} to {}", from_pin, to_pin);
      }
      if (ImNodes::IsLinkStarted(&from_pin)) {
        current_link_type = graph->get_pin_data(from_pin).type;
      }
    }

    // Handle Input
    if (focused && !io.WantTextInput) {
      if (isKeyJustPressed(ImGuiKey_Z) && io.KeyCtrl && io.KeyShift)
        history.redo();
      else if (isKeyJustPressed(ImGuiKey_Z) && io.KeyCtrl)
        history.undo();
      if (isKeyJustPressed(ImGuiKey_A) && io.KeyShift) {
        add_nodes = true;
      }
      if (isKeyJustPressed(ImGuiKey_X)) {
        int length = ImNodes::NumSelectedNodes();
        if (length > 0) {
          int nodeids[length];
          ImNodes::GetSelectedNodes(nodeids);
          for (int id : nodeids) {
            graph->delete_node(id);
          }
        }
        length = ImNodes::NumSelectedLinks();
        if (length > 0) {
          int edgeids[length];
          ImNodes::GetSelectedLinks(edgeids);
          for (int id : edgeids) {
            graph->delete_edge(id);
          }
        }
      }
    }
  }
  void render() override {
    // Do not mutate EditorContext inside NodeEditor
    std::vector<std::shared_ptr<Node>> insert_nodes = {};

    ImGui::Begin("Node Editor");
    ImNodes::EditorContextSet(context);
    ImNodes::PushColorStyle(ImNodesCol_Link,
                            Data::COLORS_HOVER[current_link_type]);
    ImNodes::BeginNodeEditor();

    Global::setUndoContext(&history);
    graph.get()->render();

    { // Add Nodes
      if (add_nodes && !ImGui::IsPopupOpen("Add Nodes"))
        ImGui::OpenPopup("Add Nodes");
      else
        add_nodes = false;

      ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4);
      if (ImGui::BeginPopup("Add Nodes")) {
        ImGui::Dummy(ImVec2(140, 0)); // Min width
        ImGui::Indent(5);

        Data::Vec2 pos = {ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y};

        if (ImGui::Selectable("Fragment Shader"))
          insert_nodes.push_back(std::make_shared<FragmentShaderNode>());
        if (ImGui::Selectable("Time"))
          insert_nodes.push_back(std::make_shared<TimeNode>());
        if (ImGui::Selectable("Viewport"))
          insert_nodes.push_back(std::make_shared<ViewportNode>());
        if (ImGui::Selectable("Texture"))
          insert_nodes.push_back(std::make_shared<Texture2DNode>());

        if (ImGui::BeginMenu("Value Nodes")) {
          ImGui::Dummy(ImVec2(150, 0)); // Min width
          ImGui::Indent(5);
          if (ImGui::Selectable("Vec2"))
            graph->insert_node(std::make_shared<Vec2Node>());
          if (ImGui::Selectable("Float"))
            graph->insert_node(std::make_shared<FloatNode>());
          ImGui::Spacing();
          ImGui::EndMenu();
        }

        ImGui::Spacing();
        ImGui::Dummy(ImVec2(0, 60));
        ImGui::EndPopup();
      }
      ImGui::PopStyleVar();
    }

    focused = ImGui::IsWindowFocused();

    ImNodes::EndNodeEditor();
    ImNodes::PopColorStyle();
    ImGui::End();

    auto io = ImGui::GetIO();
    for (auto node : insert_nodes) {
      int id = graph->insert_node(node);
      ImNodes::SetNodeScreenSpacePos(id, {io.MousePos.x, io.MousePos.y});
    }
  }
};

// Type registration
#include <cereal/archives/json.hpp>
CEREAL_REGISTER_TYPE(NodeEditorWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, NodeEditorWidget)
