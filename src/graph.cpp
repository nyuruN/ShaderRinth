#include "graph.h"
#include "nodes.h"

#include <imnodes.h>

//! RenderGraph

void RenderGraph::render() {
  for (auto &pair : nodes) { // Render Nodes
    pair.second->render(*this);
  }
  for (auto &pair : edges) { // Render Edges
    DataType type = pins.at(pair.second.from).data.type;

    ImNodes::PushColorStyle(ImNodesCol_Link, Data::COLORS[type]);
    ImNodes::PushColorStyle(ImNodesCol_LinkHovered, Data::COLORS_HOVER[type]);
    ImNodes::PushColorStyle(ImNodesCol_LinkSelected, Data::COLORS_HOVER[type]);
    ImNodes::Link(pair.first, pair.second.from, pair.second.to);
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
  }
};
int RenderGraph::insert_node(std::shared_ptr<Node> node) {
  int nodeid = get_next_node_id();
  node->id = nodeid;
  node->onEnter(*this);
  nodes.insert(std::make_pair(node->id, node));
  return nodeid;
};
int RenderGraph::insert_edge(int frompin, int topin) {
  if (pins.at(frompin).data.type != pins.at(topin).data.type) {
    spdlog::error("Not same type!");
    return -1;
  }

  { // Handle existing edge to the same pin
    int id = -1;
    for (auto &pair : edges) {
      if (pair.second.to == topin)
        id = pair.second.id;
    }
    if (id != -1)
      delete_edge(id);
  }

  Edge edge;
  edge.id = get_next_edge_id();
  edge.from = frompin;
  edge.to = topin;
  edges.insert(std::make_pair(edge.id, edge));
  return edge.id;
};
void RenderGraph::delete_node(int nodeid) {
  nodes.at(nodeid)->onExit(*this);
  nodes.erase(nodes.find(nodeid));
};
void RenderGraph::delete_pin(int pinid) {
  std::vector<int> marked;
  for (auto &pair : edges) {
    if (pair.second.from == pinid || pair.second.to == pinid) {
      marked.push_back(pair.second.id);
    }
  }
  for (auto &edgeid : marked)
    delete_edge(edgeid);
};
void RenderGraph::register_pin(int nodeid, DataType type, int *pinid) {
  Pin pin = {id : get_next_pin_id(), node_id : nodeid, data : Data(type)};
  *pinid = pin.id;
  pins.insert(std::make_pair(pin.id, pin));
};
Data RenderGraph::get_pin_data(int pinid) { return pins.at(pinid).data; };
void RenderGraph::set_pin_data(int pinid, std::any ptr) {
  std::vector<int> connected_pins;
  for (auto &pair : edges) {
    if (pair.second.from == pinid)
      connected_pins.push_back(pair.second.to);
  }
  for (auto &cpin : connected_pins) {
    this->pins.at(cpin).data.set(ptr);
  }
};
void RenderGraph::evaluate() {
  topological_order();
  should_stop = false;

  while (run_order.size() != 0) {
    int nodeid = run_order.back();
    nodes.at(nodeid)->run(*this);
    run_order.pop_back();

    if (should_stop) {
      break;
    }
  }
};
void RenderGraph::clear_graph_data() {
  run_order.clear();
  for (auto &pin : pins) {
    pin.second.data.reset();
  }
};
void RenderGraph::set_node_positions(ImNodesEditorContext *context) {
  if (!context)
    return;
  ImNodes::EditorContextSet(context);

  for (auto &pair : nodes) {
    ImVec2 pos = {pair.second->pos[0], pair.second->pos[1]};
    ImNodes::SetNodeGridSpacePos(pair.second->id, pos);
  }
}
void RenderGraph::get_node_positions() const {
  for (auto &pair : nodes) { // Save node positions
    auto v = ImNodes::GetNodeGridSpacePos(pair.second->id);
    pair.second->pos = {v.x, v.y};
  }
}
void RenderGraph::setup_nodes_on_load() {
  for (auto &pair : nodes) {
    pair.second->onLoad(*this);
  }
}
void RenderGraph::default_layout(std::shared_ptr<Shader> shader) {
  int out = insert_root_node(std::make_shared<OutputNode>());
  int time = insert_node(std::make_shared<TimeNode>());
  int viewport = insert_node(std::make_shared<ViewportNode>());
  int frag = insert_node(std::make_shared<FragmentShaderNode>());

  auto f = dynamic_cast<FragmentShaderNode *>(get_node(frag));
  auto vp = dynamic_cast<ViewportNode *>(get_node(viewport));

  int out_in = dynamic_cast<OutputNode *>(get_node(out))->get_input_pin();
  int time_out = dynamic_cast<TimeNode *>(get_node(time))->get_output_pin();
  int frag_out = f->get_output_pin();
  int vec_out = vp->get_output_pin();
  f->set_shader(shader);

  int u_res_in = f->add_uniform_pin(*this, DataType::Vec2, "u_resolution");
  int u_time_in = f->add_uniform_pin(*this, DataType::Float, "u_time");

  insert_edge(frag_out, out_in);
  insert_edge(time_out, u_time_in);
  insert_edge(vec_out, u_res_in);

  get_node(out)->pos = {500, 40};
  get_node(viewport)->pos = {20, 40};
  get_node(time)->pos = {20, 160};
  get_node(frag)->pos = {200, 60};
}
