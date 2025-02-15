#pragma once

#include "graph.h"
#include "nodes.h"
#include "widget.h"
#include <imnodes.h>

#include <cereal/types/polymorphic.hpp>

struct AddNodes {
private:
  std::vector<std::shared_ptr<Node>> insert_nodes = {};
  bool is_open = false;
  ImVec2 spawn_position = {};

public:
  AddNodes() {}
  // Opens the popup
  void open_popup() {
    is_open = true;
    spawn_position = ImGui::GetIO().MousePos;
  }
  // Apply changes to RenderGraph
  void commit(RenderGraph *graph) {
    auto io = ImGui::GetIO();
    for (auto node : insert_nodes) {
      int id = graph->insert_node(node);
      ImNodes::SetNodeScreenSpacePos(id, spawn_position);
    }
    insert_nodes.clear();
  }
  // Renders the GUI and handles user interactions
  void show();
};

struct ClipboardEdge {
  int from_node_idx, to_node_idx;
  int from_layout_idx, to_layout_idx;
};

class NodeEditorWidget : public Widget {
private:
  AssetId<RenderGraph> graph_id;
  std::shared_ptr<RenderGraph> graph = nullptr;

  ImNodesEditorContext *context = ImNodes::EditorContextCreate();
  AddNodes add_nodes = AddNodes();
  UndoContext history = UndoContext(50);
  DataType current_link_type = DataType(0); // Any default type
  bool focused = false;

  std::vector<std::shared_ptr<Node>> clipboard_nodes = {};
  std::vector<ClipboardEdge> clipboard_edges = {};
  // Call SelectNode after an object is created
  std::vector<int> deferred_node_selection = {};
  std::vector<int> deferred_edge_selection = {};
  unsigned int paste_count = 0; // For position offsets

public:
  NodeEditorWidget(int id, std::shared_ptr<AssetManager> assets, AssetId<RenderGraph> graph_id) {
    this->id = id;
    this->graph_id = graph_id;
    graph = assets->get_graph(graph_id);
  }
  void onStartup() override { graph->set_node_positions(context); };
  void onShutdown() override { ImNodes::EditorContextFree(context); }
  void onUpdate() override {
    // Apply added nodes
    add_nodes.commit(graph.get());

    // Apply deferred selection
    if (!deferred_node_selection.empty() || !deferred_edge_selection.empty()) {
      for (int id : deferred_node_selection)
        ImNodes::SelectNode(id);
      for (int id : deferred_edge_selection)
        ImNodes::SelectLink(id);
      deferred_node_selection.clear();
      deferred_edge_selection.clear();
    }

    { // ImNodes events
      int from_pin, to_pin;
      if (ImNodes::IsLinkCreated(&from_pin, &to_pin)) {
        if (graph->insert_edge(from_pin, to_pin) != -1)
          spdlog::info("Link created from {} to {}", from_pin, to_pin);
      }
      // Change styling for active started link
      if (ImNodes::IsLinkStarted(&from_pin))
        current_link_type = graph->get_pin_data(from_pin).type;
    }

    if (focused && !ImGui::GetIO().WantTextInput)
      process_input();
  }
  // Renders the widget
  void render(bool *p_open) override;
  // Handles Input
  void process_input();
  // Deletes selected links and nodes
  void delete_selected();

  template <class Archive>
  static void load_and_construct(Archive &ar, cereal::construct<NodeEditorWidget> &construct) {
    std::shared_ptr<RenderGraph> graph;
    ar(graph);
    construct(graph);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(graph)); }
  toml::table save() {
    return toml::table{
        {"type", "NodeEditorWidget"},
        {"widget_id", id},
        {"graph_id", graph_id},
    };
  }
  static NodeEditorWidget load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    int id = tbl["widget_id"].value<int>().value();
    AssetId<RenderGraph> graph_id = tbl["graph_id"].value<int>().value();
    auto w = NodeEditorWidget(id, assets, graph_id);
    return w;
  }
};

// Type registration
#include <cereal/archives/json.hpp>
// CEREAL_REGISTER_TYPE(NodeEditorWidget)
// CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, NodeEditorWidget)
