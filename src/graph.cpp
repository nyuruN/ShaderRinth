#include "nodes.h"

void RenderGraph::default_layout(std::shared_ptr<Shader> shader) {
  int out = insert_root_node(std::make_unique<OutputNode>());
  int time = insert_node(std::make_unique<TimeNode>());
  int vec = insert_node(std::make_unique<Vec2Node>());
  int frag = insert_node(std::make_unique<FragmentShaderNode>());

  auto f = dynamic_cast<FragmentShaderNode *>(get_node(frag));
  auto v = dynamic_cast<Vec2Node *>(get_node(vec));

  int out_in = dynamic_cast<OutputNode *>(get_node(out))->get_input_pin();
  int time_out = dynamic_cast<TimeNode *>(get_node(time))->get_output_pin();
  int frag_out = f->get_output_pin();
  int vec_out = v->get_output_pin();
  f->set_shader(shader);

  int u_res_in = f->add_uniform_pin(*this, DataType::Vec2, "u_resolution");
  int u_time_in = f->add_uniform_pin(*this, DataType::Float, "u_time");
  v->set_value(640.0f, 480.0f);

  insert_edge(frag_out, out_in);
  insert_edge(time_out, u_time_in);
  insert_edge(vec_out, u_res_in);

  ImNodes::SetNodeGridSpacePos(out, ImVec2(500, 40));
  ImNodes::SetNodeGridSpacePos(vec, ImVec2(20, 40));
  ImNodes::SetNodeGridSpacePos(time, ImVec2(20, 160));
  ImNodes::SetNodeGridSpacePos(frag, ImVec2(200, 60));
}
