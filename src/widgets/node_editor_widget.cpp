#include "widgets/node_editor_widget.h"
#include <algorithm>

//! AddNodes

void AddNodes::commit(RenderGraph *graph) {
  auto io = ImGui::GetIO();
  for (auto node : insert_nodes) {
    int id = graph->insert_node(node);
    ImNodes::SetNodeScreenSpacePos(id, spawn_position);
  }
  insert_nodes.clear();
}
void AddNodes::show(std::shared_ptr<AssetManager> assets) {
  if (is_open && !ImGui::IsPopupOpen("Add Nodes"))
    ImGui::OpenPopup("Add Nodes");
  else
    is_open = false;

  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4);
  if (ImGui::BeginPopup("Add Nodes")) {
    ImGui::Dummy(ImVec2(140, 0)); // Min width
    ImGui::Indent(5);

    if (ImGui::Selectable("Fragment Shader"))
      insert_nodes.push_back(std::make_shared<FragmentShaderNode>());
    if (ImGui::Selectable("Time"))
      insert_nodes.push_back(std::make_shared<TimeNode>());
    if (ImGui::Selectable("Viewport"))
      insert_nodes.push_back(std::make_shared<ViewportNode>());
    if (ImGui::Selectable("Texture"))
      insert_nodes.push_back(std::make_shared<Texture2DNode>(assets));

    if (ImGui::BeginMenu("Value Nodes")) {
      ImGui::Dummy(ImVec2(150, 0)); // Min width
      ImGui::Indent(5);
      if (ImGui::Selectable("Vec2"))
        insert_nodes.push_back(std::make_shared<Vec2Node>());
      if (ImGui::Selectable("Float"))
        insert_nodes.push_back(std::make_shared<FloatNode>());
      ImGui::Spacing();
      ImGui::EndMenu();
    }

    ImGui::Spacing();
    ImGui::Dummy(ImVec2(0, 60));
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();
}

//! Clipboard

void Clipboard::apply_deferred() {
  if (!deferred_node_selection.empty() || !deferred_edge_selection.empty()) {
    for (int id : deferred_node_selection)
      ImNodes::SelectNode(id);
    for (int id : deferred_edge_selection)
      ImNodes::SelectLink(id);
    deferred_node_selection.clear();
    deferred_edge_selection.clear();
  }
}
void Clipboard::copy(RenderGraph *graph) {
  clipboard_nodes.clear();
  clipboard_edges.clear();
  int length;

  if ((length = ImNodes::NumSelectedNodes()) > 0) {
    int nodeids[length];
    ImNodes::GetSelectedNodes(nodeids);
    for (auto &id : nodeids) {
      auto node = graph->get_node(id)->clone();
      auto pos = ImNodes::GetNodeGridSpacePos(id);
      node->pos = {pos.x, pos.y};
      clipboard_nodes.push_back(node);
    }
  }
  if ((length = ImNodes::NumSelectedLinks()) > 0) {
    int edgeids[length];
    ImNodes::GetSelectedLinks(edgeids);
    for (auto &id : edgeids) {
      Edge edge = graph->get_edge(id);
      Pin from = graph->get_pin(edge.from);
      Pin to = graph->get_pin(edge.to);

      // Break if only one or none of the connected nodes are selected
      if (!(ImNodes::IsNodeSelected(from.node_id) && ImNodes::IsNodeSelected(to.node_id)))
        break;

      // Find the matching node index in clipboard_nodes
      auto from_it =
          std::find_if(clipboard_nodes.begin(), clipboard_nodes.end(),
                       [from](std::shared_ptr<Node> node) { return node->id == from.node_id; });
      auto to_it =
          std::find_if(clipboard_nodes.begin(), clipboard_nodes.end(),
                       [to](std::shared_ptr<Node> node) { return node->id == to.node_id; });

      int from_node_idx = std::distance(clipboard_nodes.begin(), from_it);
      int to_node_idx = std::distance(clipboard_nodes.begin(), to_it);

      auto from_layout = clipboard_nodes[from_node_idx]->layout();
      auto to_layout = clipboard_nodes[to_node_idx]->layout();

      // Find matching layout index for our pins
      auto from_layout_it = std::find(from_layout.begin(), from_layout.end(), edge.from);
      auto to_layout_it = std::find(to_layout.begin(), to_layout.end(), edge.to);
      int from_layout_idx = std::distance(from_layout.begin(), from_layout_it);
      int to_layout_idx = std::distance(to_layout.begin(), to_layout_it);

      clipboard_edges.push_back(ClipboardEdge{
          from_node_idx,
          to_node_idx,
          from_layout_idx,
          to_layout_idx,
      });
    }
  }

  paste_count = 0;
}
void Clipboard::paste(RenderGraph *graph) {
  if (clipboard_nodes.empty() && clipboard_edges.empty())
    return;

  static constexpr int OFFSET[2] = {20, 20};
  ImNodes::ClearNodeSelection();
  ImNodes::ClearLinkSelection();

  // inserted_nodes should be the same as clipboard_nodes
  // but initialized and integrated with a RenderGraph
  std::vector<std::shared_ptr<Node>> inserted_nodes;

  for (auto &clipboard_node : clipboard_nodes) {
    auto node = clipboard_node->clone();
    graph->insert_node(node);
    ImNodes::SetNodeGridSpacePos(node->id, {node->pos[0] + OFFSET[0] * (paste_count + 1),
                                            node->pos[1] + OFFSET[1] * (paste_count + 1)});

    inserted_nodes.push_back(node);
    deferred_node_selection.push_back(node->id);
  }
  for (auto clipboard_edge : clipboard_edges) {
    auto from = inserted_nodes[clipboard_edge.from_node_idx];
    auto to = inserted_nodes[clipboard_edge.to_node_idx];

    auto from_layout = from->layout();
    auto to_layout = to->layout();

    auto from_pin = from_layout[clipboard_edge.from_layout_idx];
    auto to_pin = to_layout[clipboard_edge.to_layout_idx];

    int id = graph->insert_edge(from_pin, to_pin);

    deferred_edge_selection.push_back(id);
  }

  paste_count++;
}

//! NodeEditorWidget

void NodeEditorWidget::render(bool *) {
  ImGui::Begin(title.c_str());
  ImNodes::EditorContextSet(context);
  ImNodes::PushColorStyle(ImNodesCol_Link, Data::COLORS_HOVER[current_link_type]);
  ImNodes::BeginNodeEditor();

  Global::setUndoContext(&history);
  graph.get()->render();

  if (auto assets = this->assets.lock())
    add_nodes.show(assets);

  focused = ImGui::IsWindowFocused();

  ImNodes::EndNodeEditor();
  ImNodes::PopColorStyle();
  ImGui::End();
}
void NodeEditorWidget::process_input() {
  auto io = ImGui::GetIO();

  if (isKeyJustPressed(ImGuiKey_Z) && io.KeyCtrl && io.KeyShift)
    history.redo();
  else if (isKeyJustPressed(ImGuiKey_Z) && io.KeyCtrl)
    history.undo();
  if (isKeyJustPressed(ImGuiKey_A) && io.KeyShift)
    add_nodes.open_popup();
  if (io.MouseClicked[1]) // Right-click
    add_nodes.open_popup();
  if (isKeyJustPressed(ImGuiKey_X))
    delete_selected();
  if (isKeyJustPressed(ImGuiKey_C) && io.KeyCtrl)
    clipboard.copy(graph.get());
  if (isKeyJustPressed(ImGuiKey_V) && io.KeyCtrl)
    clipboard.paste(graph.get());
}
void NodeEditorWidget::delete_selected() {
  int length;

  // Delete links
  if ((length = ImNodes::NumSelectedLinks()) > 0) {
    int edgeids[length];
    ImNodes::GetSelectedLinks(edgeids);
    for (int id : edgeids) {
      if (ImNodes::IsLinkSelected(id))
        ImNodes::ClearLinkSelection(id);
      graph->delete_edge(id);
    }
  }
  // Delete nodes
  if ((length = ImNodes::NumSelectedNodes()) > 0) {
    int nodeids[length];
    ImNodes::GetSelectedNodes(nodeids);
    for (int id : nodeids) {
      if (ImNodes::IsNodeSelected(id))
        ImNodes::ClearNodeSelection(id);
      graph->delete_node(id);
    }
  }
}
