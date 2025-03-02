#pragma once

#include "nodes.h"
#include "widget.h"
#include <imnodes.h>

// Forward declares
struct AssetManager;

// Helper struct for adding nodes
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
  void commit(RenderGraph *graph);
  // Renders the GUI and handles user interactions
  void show(std::shared_ptr<AssetManager>);
};

// Helper struct for copy and paste
struct Clipboard {
private:
  struct ClipboardEdge {
    int from_node_idx, to_node_idx;
    int from_layout_idx, to_layout_idx;
  };

  std::vector<std::shared_ptr<Node>> clipboard_nodes = {};
  std::vector<ClipboardEdge> clipboard_edges = {};
  // Call ImNodes::SelectNode() after an object is created
  std::vector<int> deferred_node_selection = {};
  std::vector<int> deferred_edge_selection = {};
  unsigned int paste_count = 0; // For position offsets

public:
  // Apply deferred selection
  void apply_deferred();
  // Copy currently selected Nodes and Edges
  void copy(RenderGraph *graph);
  // Paste stored Nodes and Edges (if any)
  void paste(RenderGraph *graph);
};

class NodeEditorWidget : public Widget {
private:
  AssetId<RenderGraph> graph_id;
  std::shared_ptr<RenderGraph> graph = nullptr;
  std::weak_ptr<AssetManager> assets;
  std::string title;

  ImNodesEditorContext *context = ImNodes::EditorContextCreate();
  AddNodes add_nodes = AddNodes();
  Clipboard clipboard = Clipboard();
  UndoContext history = UndoContext(50);
  DataType current_link_type = DataType(0); // Any default type
  bool focused = false;

public:
  NodeEditorWidget(int id, std::shared_ptr<AssetManager> assets, AssetId<RenderGraph> graph_id) {
    this->id = id;
    this->graph_id = graph_id;
    this->graph = assets->get_graph(graph_id);
    this->assets = assets;
    this->title = fmt::format("Node Editor##{}", id);
  }
  void onStartup() override { graph->set_node_positions(context); };
  void onShutdown() override { ImNodes::EditorContextFree(context); }
  void onUpdate() override {
    // Apply added nodes
    add_nodes.commit(graph.get());

    // Apply deferred selection
    clipboard.apply_deferred();

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

  toml::table save() {
    return toml::table{
        {"type", "NodeEditorWidget"},
        {"widget_id", id},
        {"graph_id", graph_id},
    };
  }
  static std::shared_ptr<Widget> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    int id = tbl["widget_id"].value<int>().value();
    AssetId<RenderGraph> graph_id = tbl["graph_id"].value<int>().value();
    auto w = NodeEditorWidget(id, assets, graph_id);
    return std::make_shared<NodeEditorWidget>(w);
  }
};

REGISTER_WIDGET_FACTORY(NodeEditorWidget);
