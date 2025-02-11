#include "widgets/node_editor_widget.h"

//! AddNodes

void AddNodes::show() {
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
      insert_nodes.push_back(std::make_shared<Texture2DNode>());

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

//! NodeEditorWidget

void NodeEditorWidget::render() {
  ImGui::Begin("Node Editor");
  ImNodes::EditorContextSet(context);
  ImNodes::PushColorStyle(ImNodesCol_Link,
                          Data::COLORS_HOVER[current_link_type]);
  ImNodes::BeginNodeEditor();

  Global::setUndoContext(&history);
  graph.get()->render();

  add_nodes.show();

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
  if (isKeyJustPressed(ImGuiKey_C) && io.KeyCtrl) {
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
        // TODO: Copy Logic
      }
    }

    paste_count = 0;
  }
  if (isKeyJustPressed(ImGuiKey_V) && io.KeyCtrl) {
    static constexpr int OFFSET[2] = {20, 20};
    ImNodes::ClearNodeSelection();
    ImNodes::ClearLinkSelection();

    for (auto &clipboard_node : clipboard_nodes) {
      auto node = clipboard_node->clone();
      graph->insert_node(node);
      ImNodes::SetNodeGridSpacePos(
          node->id, {node->pos[0] + OFFSET[0] * (paste_count + 1),
                     node->pos[1] + OFFSET[1] * (paste_count + 1)});
      deferred_node_selection.push_back(node->id);
    }

    paste_count++;
  }
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
