#include "imnodes/imnodes.h"
#include "utils.h"
#include <GLFW/glfw3.h>
#include <any>
#include <cstdlib>
#include <imgui.h>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>

// DataType:
// Type of the data transferred between nodes
enum DataType {
  Int,
  IVec2,
  IVec3,
  IVec4,
  UInt,
  UVec2,
  UVec3,
  UVec4,
  Float,
  Vec2,
  Vec3,
  Vec4,
  Texture2D,
};

const std::map<DataType, std::string> TYPESTRMAP = {
    {DataType::Int, "Int"},
    {DataType::IVec2, "IVec2"},
    {DataType::IVec3, "IVec3"},
    {DataType::IVec4, "IVec4"},
    {DataType::UInt, "UInt"},
    {DataType::UVec2, "UVec2"},
    {DataType::UVec3, "UVec3"},
    {DataType::UVec4, "UVec4"},
    {DataType::Float, "Float"},
    {DataType::Vec2, "Vec2"},
    {DataType::Vec3, "Vec3"},
    {DataType::Vec4, "Vec4"},
    {DataType::Texture2D, "Texture2D"},
};

const std::string to_string(DataType type) noexcept;

struct RenderGraph;

// Nodes:
// This base class defines function required for
// integration within the RenderGraph
class Node {
public:
  int id;

  // Renders the Node
  virtual void render(RenderGraph &) {};
  virtual void onEnter(RenderGraph &) {};
  virtual void onExit(RenderGraph &) {};
  // Runs the Node under the assumption
  // that all leaf nodes are satisfied
  // and writes to output pins
  virtual void run(RenderGraph &) {};
};

// RenderGraph
struct RenderGraph {
  // Represents a pin of a node
  struct Pin {
    int node_id;
    int id;
    DataType type;
    std::any data;
  };
  // Represents a connection between nodes
  struct Edge {
    int id;
    int from, to;
  };
  std::map<int, std::unique_ptr<Node>> nodes;
  std::map<int, Edge> edges;
  std::map<int, Pin> pins;

  std::shared_ptr<Assets<Shader>> shaders;
  std::shared_ptr<Geometry> graph_geometry = nullptr;
  ImVec2 viewport_resolution = ImVec2(640, 480);
  int root_node;

  std::vector<int> run_order;
  bool should_stop = false;

  RenderGraph(std::shared_ptr<Assets<Shader>> shaders) {
    this->shaders = shaders;
  };
  void set_resolution(ImVec2 res) { viewport_resolution = res; }
  void set_geometry(std::shared_ptr<Geometry> geo) { graph_geometry = geo; }
  int get_next_node_id() {
    static int id = 0;
    return id++;
  };
  int get_next_edge_id() {
    static int id = 0;
    return id++;
  };
  int get_next_pin_id() {
    static int id = 0;
    return id++;
  };
  int insert_root_node(std::unique_ptr<Node> node) {
    int nodeid = this->insert_node(std::move(node));
    this->root_node = nodeid;
    return nodeid;
  };
  int insert_node(std::unique_ptr<Node> node) {
    int nodeid = get_next_node_id();
    node->id = nodeid;
    node->onEnter(*this);
    nodes.insert(std::make_pair(node->id, std::move(node)));
    return nodeid;
  };
  void register_pin(int nodeid, DataType type, int *pinid) {
    Pin pin;
    pin.type = type;
    pin.node_id = nodeid;
    pin.id = this->get_next_pin_id();
    *pinid = pin.id;
    pins.insert(std::make_pair(pin.id, pin));
  };
  int insert_edge(int frompin, int topin) {
    if (this->pins[frompin].type != this->pins[topin].type) {
      spdlog::error("Not same type!");
      return -1;
    }

    Edge edge;
    edge.id = get_next_edge_id();
    edge.from = frompin;
    edge.to = topin;
    spdlog::debug("link created from {} to {}", frompin, topin);
    edges.insert(std::make_pair(edge.id, edge));
    return edge.id;
  };
  // Removes all traces of a node from graph including edges pins etc.
  void delete_node(int nodeid) {
    nodes.at(nodeid)->onExit(*this);
    nodes.erase(nodes.find(nodeid));
  };
  void delete_pin(int pinid) {
    std::vector<int> marked;
    for (auto &pair : edges) {
      if (pair.second.from == pinid || pair.second.to == pinid) {
        marked.push_back(pinid);
      }
    }
    for (auto &edgeid : marked)
      delete_edge(edgeid);
  };
  void delete_edge(int edgeid) { edges.erase(edges.find(edgeid)); };
  void render() {
    ImNodes::BeginNodeEditor();

    for (auto &pair : nodes) { // Render Nodes
      pair.second->render(*this);
    }
    for (auto &pair : edges) { // Render Edges
      ImNodes::Link(pair.first, pair.second.from, pair.second.to);
    }

    ImNodes::EndNodeEditor();
  };
  void get_pin_data(int pinid, std::any *ptr) { *ptr = pins.at(pinid).data; };
  void set_pin_data(int pinid, std::any ptr) {
    std::vector<int> connected_pins;
    for (auto &pair : this->edges) {
      if (pair.second.from == pinid)
        connected_pins.push_back(pair.second.to);
    }
    for (auto &cpin : connected_pins) {
      this->pins[cpin].data = ptr;
    }
  };
  void get_pins(int nodeid, std::vector<int> &pins) {
    for (auto &pair : this->pins) {
      if (pair.second.node_id == nodeid)
        pins.push_back(pair.first);
    }
  };
  void get_children(int nodeid, std::vector<int> &children) {
    std::vector<int> pins;
    get_pins(nodeid, pins);

    for (auto &epair : this->edges) {
      for (auto &pin : pins) {
        // output pin connected to some other node
        if (pin == epair.second.to) {
          children.push_back(this->pins[epair.second.from].node_id);
        }
      }
    }
  }
  // Starting from the root node, traverse the N-ary tree
  // to perform a topological sort
  void traverse(std::vector<int> &order, int nodeid) {
    order.push_back(nodeid);

    std::vector<int> children;
    get_children(nodeid, children);

    for (auto &child : children) {
      traverse(order, child);
    }
  }
  void topological_order() { traverse(run_order, root_node); };
  void evaluate() {
    topological_order();
    should_stop = false;

    while (run_order.size() != 0) {
      int nodeid = run_order.back();
      nodes[nodeid]->run(*this);
      run_order.pop_back();

      if (should_stop) {
        break;
      }
    }
  };
  Node *get_node(int nodeid) { return nodes[nodeid].get(); }
  Node *get_root_node() { return nodes[root_node].get(); }
  // Prevents nodes from reusing previous data
  void clear_graph_data() {
    run_order.clear();
    for (auto &pin : pins) {
      pin.second.data = nullptr;
    }
  };
};

// ============
// Custom Nodes
// ============

class OutputNode : public Node {
  int input_pin;
  GLuint out_texture;

  float node_width = 80.0f;

public:
  int get_input_pin() { return input_pin; }
  GLuint get_image() { return out_texture; }
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Output");
    ImNodes::EndNodeTitleBar();
    {
      ImNodes::BeginInputAttribute(input_pin);
      ImGui::Text("Image");
      ImNodes::EndInputAttribute();
    }
    ImGui::Dummy(ImVec2(node_width, 20.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Texture2D, &input_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(input_pin); }
  void run(RenderGraph &graph) override {
    std::any data;
    graph.get_pin_data(input_pin, &data);
    try {
      out_texture = std::any_cast<int>(data);
    } catch (std::bad_any_cast) {
      spdlog::error("Nothing is connected!");
    }
  }
};
class TimeNode : public Node {
  int output_pin;

  float node_width = 80.0f;

public:
  int get_output_pin() { return output_pin; }
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Time");
    ImNodes::EndNodeTitleBar();
    {
      ImNodes::BeginOutputAttribute(output_pin);
      ImGui::Indent(node_width - ImGui::CalcTextSize("Float").x);
      ImGui::Text("Float");
      ImNodes::EndOutputAttribute();
    }
    ImGui::Dummy(ImVec2(node_width, 10.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Float, &output_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override {
    graph.set_pin_data(output_pin, (float)glfwGetTime());
  }
};
class FloatNode : public Node {
  int output_pin;
  float value = 0.0f;

  float node_width = 80.0f;

public:
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Float");
    ImNodes::EndNodeTitleBar();
    {
      ImNodes::BeginOutputAttribute(output_pin);
      ImGui::SetNextItemWidth(node_width);
      ImGui::InputFloat("Float", &value, 0, 0, "%.2f");
      ImNodes::EndOutputAttribute();
    }
    ImGui::Dummy(ImVec2(node_width, 20));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Float, &output_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override {
    graph.set_pin_data(output_pin, (float)value);
  }
};
class Vec2Node : public Node {
  int output_pin;
  float value[2] = {0};

  float node_width = 120.0f;

public:
  void set_value(float x, float y) {
    value[0] = x;
    value[1] = y;
  }
  int get_output_pin() { return output_pin; }
  void render(RenderGraph &graph) override {
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Vec2");
    ImNodes::EndNodeTitleBar();
    {
      ImNodes::BeginOutputAttribute(output_pin);
      ImGui::SetNextItemWidth(node_width);
      ImGui::InputFloat2("##hidelabel", value, "%.1f");
      ImNodes::EndOutputAttribute();
    }
    ImGui::Dummy(ImVec2(node_width, 20));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Vec2, &output_pin);
  }
  void onExit(RenderGraph &graph) override { graph.delete_pin(output_pin); }
  void run(RenderGraph &graph) override {
    graph.set_pin_data(output_pin, (float *)value);
  }
};
class FragmentShaderNode : public Node {
  struct UniformPin {
    int pinid;
    DataType type;
    std::string identifier;
  };

  int output_pin;
  std::vector<UniformPin> uniform_pins;
  char shader_name[128] = {"Default\0"};
  GLuint image_fbo;
  GLuint image_colorbuffer;
  GLuint bound_textures = 0;

  const float node_width = 240.0f;

public:
  int get_output_pin() { return output_pin; }
  int add_uniform_pin(RenderGraph &graph, DataType type, std::string name) {
    int pinid;
    graph.register_pin(id, type, &pinid);
    uniform_pins.push_back(UniformPin{
      pinid,
      type,
      identifier : name,
    });
    return pinid;
  }
  void render(RenderGraph &graph) override {
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
    ImGui::InputText("##hidelabel", shader_name, sizeof(shader_name));

    {
      ImNodes::BeginOutputAttribute(output_pin);
      ImGui::Indent(node_width - ImGui::CalcTextSize("Image").x);
      ImGui::Text("Image");
      ImNodes::EndOutputAttribute();
    }

    if (ImGui::BeginPopup("add_uniform_popup")) { // Creation logic
      ImGui::SeparatorText("Uniforms");
      for (auto &pair : TYPESTRMAP) {
        if (ImGui::MenuItem(pair.second.c_str())) {
          add_uniform_pin(graph, pair.first, fmt::format("u_{}", pair.second));
        }
      }
      ImGui::EndPopup();
    }

    { // Render uniforms + delete logic
      std::vector<int> marked;
      int idx = 0;
      for (auto &pin : uniform_pins) {
        ImNodes::BeginInputAttribute(pin.pinid);
        ImGui::Text(to_string(pin.type).c_str());
        ImGui::SameLine();

        // TODO I should optimize this someday :(
        std::vector<char> buf(pin.identifier.begin(), pin.identifier.end());
        buf.push_back('\0');
        ImGui::SetNextItemWidth(
            node_width - 20 - ImGui::CalcTextSize(" - ").x -
            ImGui::CalcTextSize(to_string(pin.type).c_str()).x);
        ImGui::InputText("##hidelabel", buf.data(), sizeof(buf));
        pin.identifier = std::string(buf.data());

        ImGui::SameLine();
        ImGui::Indent(node_width - ImGui::CalcTextSize(" - ").x);
        if (ImGui::Button(" - ")) {
          marked.push_back(pin.pinid);
        }
        ImNodes::EndInputAttribute();
        idx++;
      }
      for (auto &idx : marked) {
        graph.delete_pin(uniform_pins[idx].pinid);
        uniform_pins.erase(uniform_pins.begin() + idx);
      }
    }

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(id, DataType::Texture2D, &this->output_pin);

    glGenFramebuffers(1, &image_fbo);
    glGenTextures(1, &image_colorbuffer);

    glBindTexture(GL_TEXTURE_2D, image_colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, graph.viewport_resolution.x,
                 graph.viewport_resolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  void onExit(RenderGraph &graph) override {
    graph.delete_pin(output_pin);
    for (auto &pin : uniform_pins) {
      graph.delete_pin(pin.pinid);
    }
    glDeleteFramebuffers(1, &image_fbo);
    glDeleteTextures(1, &image_colorbuffer);
  }
  void run(RenderGraph &graph) override {
    std::shared_ptr<Shader> shader;

    try {
      auto str = std::string(shader_name);
      shader = graph.shaders->at(str);
    } catch (std::out_of_range) {
      graph.should_stop = true;
      spdlog::error("Shader \"{}\" does not exist!", shader_name);
      return;
    }

    if (!shader->is_compiled()) {
      if (!shader->compile(graph.graph_geometry)) {
        spdlog::error(shader->get_log());
        graph.should_stop = true;
        return;
      }
    }

    glBindTexture(GL_TEXTURE_2D, image_colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, graph.viewport_resolution.x,
                 graph.viewport_resolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (auto &pin : uniform_pins) {
      std::any data;
      graph.get_pin_data(pin.pinid, &data);
      set_uniform(*shader.get(), pin.identifier, pin.type, data);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, image_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           image_colorbuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      graph.should_stop = true;
      spdlog::error("Framebuffer is not complete!");
      return;
    }

    glViewport(0, 0, graph.viewport_resolution.x, graph.viewport_resolution.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader->use();
    graph.graph_geometry->draw_geometry();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    bound_textures = 0;

    graph.set_pin_data(output_pin, (int)image_colorbuffer);
  }
  unsigned int get_next_texture_unit() {
    unsigned int unit = bound_textures;
    bound_textures++;
    return GL_TEXTURE0 + unit;
  }
  void set_uniform(Shader &shader, std::string identifier, DataType type,
                   std::any data) {
    GLuint loc = shader.get_uniform_loc(identifier.data());
    // debugAny(data);

    switch (type) {
    case DataType::Int: {
      int value = std::any_cast<int>(data);
      glUniform1iv(loc, 1, &value);
      break;
    };
    case DataType::IVec2: {
      int *value = std::any_cast<int *>(data);
      glUniform2iv(loc, 1, value);
      break;
    };
    case DataType::IVec3: {
      int *value = std::any_cast<int *>(data);
      glUniform3iv(loc, 1, value);
      break;
    };
    case DataType::IVec4: {
      int *value = std::any_cast<int *>(data);
      glUniform4iv(loc, 1, value);
      break;
    };
    case DataType::UInt: {
      unsigned int value = std::any_cast<unsigned int>(data);
      glUniform1uiv(loc, 1, &value);
      break;
    };
    case DataType::UVec2: {
      unsigned int *value = std::any_cast<unsigned int *>(data);
      glUniform2uiv(loc, 1, value);
      break;
    };
    case DataType::UVec3: {
      unsigned int *value = std::any_cast<unsigned int *>(data);
      glUniform3uiv(loc, 1, value);
      break;
    };
    case DataType::UVec4: {
      unsigned int *value = std::any_cast<unsigned int *>(data);
      glUniform4uiv(loc, 1, value);
      break;
    };
    case DataType::Float: {
      float value = std::any_cast<float>(data);
      glUniform1fv(loc, 1, &value);
      break;
    };
    case DataType::Vec2: {
      float *value = std::any_cast<float *>(data);
      glUniform2fv(loc, 1, value);
      break;
    };
    case DataType::Vec3: {
      float *value = std::any_cast<float *>(data);
      glUniform3fv(loc, 1, value);
      break;
    };
    case DataType::Vec4: {
      float *value = std::any_cast<float *>(data);
      glUniform4fv(loc, 1, value);
      break;
    };
    case DataType::Texture2D: {
      GLuint texture = std::any_cast<unsigned int>(data);
      int unit = get_next_texture_unit();
      glActiveTexture(unit);
      glBindTexture(GL_TEXTURE_2D, texture);
      glUniform1i(loc, texture);
      break;
    };
    }
  }
};
