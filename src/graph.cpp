#include "nodes.h"

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
