#include "imnodes/imnodes.h"
#include <GLFW/glfw3.h>
#include <any>
#include <cstdlib>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <utility>

// DataType:
// Type of the data transferred between nodes
enum DataType {
  Int,
  Float,
  Vec2,
  Vec3,
  Vec4,
  Texture2D,
};

struct RenderGraph;
class Node;

// Nodes:
// This base class defines function required for
// integration within the RenderGraph
class Node {
public:
  int id;

  // Renders the Node
  virtual void render() {};
  virtual void onEnter(RenderGraph &) {};
  virtual void onExit(RenderGraph &) {};
  // Runs the Node under the assumption
  // that all depencies are fulfilled
  // and writes to output pins
  virtual void run(RenderGraph &) {};
};

// RenderGraph
struct RenderGraph {
  // Represents a pin of a node
  struct Pin {
    int node_id;
    int id;
    DataType type; // This must be the same in a link
    std::any data;
  };
  // Represents a connection between nodes
  struct Edge {
    int id;
    int from, to;
  };
  // Unique ptr since we need polymorphism
  std::map<int, std::unique_ptr<Node>> nodes;
  std::map<int, Edge> edges;
  std::map<int, Pin> pins;
  int root_node;

  std::vector<int> run_order;

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
    // TODO
  };
  void delete_edge(int edgeid) {
    // TODO
  }
  void render() {
    // Render Nodes
    for (auto &pair : nodes) {
      pair.second->render();
    }
    // Render Edges
    for (auto &pair : edges) {
      ImNodes::Link(pair.first, pair.second.from, pair.second.to);
    }
  };
  void get_pin_data(int pinid, std::any *ptr) { *ptr = pins.at(pinid).data; };
  void set_pin_data(int pinid, std::any ptr) {
    this->pins.at(pinid).data = ptr;
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
  // to put them in topological order
  void traverse(std::vector<int> &order, int nodeid) {
    order.push_back(nodeid);

    std::vector<int> children;
    get_children(nodeid, children);

    for (auto &child : children) {
      traverse(order, child);
    }
  }
  void topological_order() { traverse(this->run_order, this->root_node); };
  void evaluate() {
    topological_order();

    while (this->run_order.size() != 0) {
      int nodeid = this->run_order.back();
      spdlog::debug("running node {}", nodeid);
      this->nodes[nodeid]->run(*this);
      this->run_order.pop_back();
    }
  };
  Node *get_root_node() { return this->nodes[this->root_node].get(); }
  void clear_graph() {
    this->run_order.clear();
    for (auto &pin : this->pins) {
      pin.second.data = nullptr;
    }
  };
};

// ============
// Custom Nodes
// ============

class OutputNode : public Node {
  int input_pin;
  int out_texture;

public:
  void render() override {
    ImNodes::BeginNode(this->id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Output");
    ImNodes::EndNodeTitleBar();
    {
      ImNodes::BeginInputAttribute(this->input_pin);
      ImGui::Text("Image");
      ImNodes::EndInputAttribute();
    }
    ImGui::Dummy(ImVec2(80.0f, 45.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(this->id, DataType::Texture2D, &this->input_pin);
  }
  void run(RenderGraph &graph) override {
    std::any data;
    graph.get_pin_data(this->input_pin, &data);
    this->out_texture = std::any_cast<int>(data);
  }
};
class TimeNode : public Node {
  int output_pin;

public:
  void render() override {
    ImNodes::BeginNode(this->id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Time");
    ImNodes::EndNodeTitleBar();
    {
      ImNodes::BeginOutputAttribute(this->output_pin);
      ImGui::Text("Float");
      ImNodes::EndOutputAttribute();
    }
    ImGui::Dummy(ImVec2(80.0f, 45.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(this->id, DataType::Float, &this->output_pin);
  }
  void run(RenderGraph &graph) override {
    std::any data = glfwGetTime();
    graph.set_pin_data(this->output_pin, data);
  }
};
class FloatOutputNode : public Node {
  int input_pin;
  float value;

public:
  void render() override {
    ImNodes::BeginNode(this->id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Output");
    ImNodes::EndNodeTitleBar();
    {
      ImNodes::BeginInputAttribute(this->input_pin);
      ImGui::Text("Float");
      ImNodes::EndInputAttribute();
    }
    ImGui::Dummy(ImVec2(80.0f, 45.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(this->id, DataType::Float, &this->input_pin);
  }
  void run(RenderGraph &graph) override {
    std::any data;
    graph.get_pin_data(this->input_pin, &data);
    this->value = std::any_cast<float>(data);
  }
  float get_value() { return this->value; };
};
class FloatNode : public Node {
  int output_pin;
  float value;

public:
  void render() override {
    ImNodes::BeginNode(this->id);

    ImNodes::BeginNodeTitleBar();
    ImGui::Text("Float");
    ImNodes::EndNodeTitleBar();
    {
      ImNodes::BeginOutputAttribute(this->output_pin);
      ImGui::Text("Float");
      ImGui::SameLine();
      ImGui::InputFloat("##hidelabel", &value, 0.1);
      ImNodes::EndOutputAttribute();
    }
    ImGui::Dummy(ImVec2(80.0f, 45.0f));

    ImNodes::EndNode();
  }
  void onEnter(RenderGraph &graph) override {
    graph.register_pin(this->id, DataType::Float, &this->output_pin);
  }
  void run(RenderGraph &graph) override {
    std::any data = this->value;
    graph.set_pin_data(this->output_pin, data);
  }
};
