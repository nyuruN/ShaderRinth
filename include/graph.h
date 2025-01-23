#pragma once

#include "imnodes.h"
#include "material.h"
#include "utils.h"
#include <GLES3/gl3.h>
#include <any>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>

// DataType:
// Type of the data transferred between nodes
enum DataType {
  Int,       // int
  IVec2,     // std::array<int, 2>
  IVec3,     // std::array<int, 2>
  IVec4,     // std::array<int, 2>
  UInt,      // uint
  UVec2,     // std::array<uint, 2>
  UVec3,     // std::array<uint, 2>
  UVec4,     // std::array<uint, 2>
  Float,     // float
  Vec2,      // std::array<float, 2>
  Vec3,      // std::array<float, 2>
  Vec4,      // std::array<float, 2>
  Texture2D, // GLuint
};

struct Data {
  using Int = int;
  using IVec2 = std::array<int, 2>;
  using IVec3 = std::array<int, 2>;
  using IVec4 = std::array<int, 2>;
  using UInt = uint;
  using UVec2 = std::array<uint, 2>;
  using UVec3 = std::array<uint, 2>;
  using UVec4 = std::array<uint, 2>;
  using Float = float;
  using Vec2 = std::array<float, 2>;
  using Vec3 = std::array<float, 2>;
  using Vec4 = std::array<float, 2>;
  using Texture2D = GLuint;

  constexpr static DataType ALL[] = {
      DataType::Int,       DataType::IVec2, DataType::IVec3, DataType::IVec4,
      DataType::UInt,      DataType::UVec2, DataType::UVec3, DataType::UVec4,
      DataType::Float,     DataType::Vec2,  DataType::Vec3,  DataType::Vec4,
      DataType::Texture2D,
  };

  DataType type;
  std::any data;

  Data(DataType type = DataType(-1), std::any data = nullptr)
      : type(type), data(data) {}
  constexpr bool operator==(DataType t) { return type == t; }
  operator bool() const { return data.has_value(); }

  template <typename T> T get() { return std::any_cast<T>(data); }
  void set(std::any d) { data = d; }
  void reset() { data.reset(); }
  template <class Archive> void serialize(Archive &ar) { ar(CEREAL_NVP(type)); }
  constexpr static char *type_name(DataType type) {
    // clang-format off
    switch (type) {
      case DataType::Int:       return "Int";
      case DataType::IVec2:     return "IVec2";
    	case DataType::IVec3:     return "IVec3";
    	case DataType::IVec4:     return "IVec4";
    	case DataType::UInt:      return "UInt";
    	case DataType::UVec2:     return "UVec2";
    	case DataType::UVec3:     return "UVec3";
    	case DataType::UVec4:     return "UVec4";
    	case DataType::Float:     return "Float";
    	case DataType::Vec2:      return "Vec2";
    	case DataType::Vec3:      return "Vec3";
    	case DataType::Vec4:      return "Vec4";
    	case DataType::Texture2D: return "Texture2D";
      default: throw std::runtime_error("invalid data type!");
    }
    // clang-format on
  }
  char *type_name() { return type_name(type); }
};

struct RenderGraph;

// Nodes:
// This base class defines function required for
// integration within the RenderGraph
class Node {
public:
  int id = -1;
  Data::Vec2 pos = {};

  // Renders the Node
  virtual void render(RenderGraph &) {}
  virtual void onEnter(RenderGraph &) {}
  virtual void onExit(RenderGraph &) {}
  // Called when the Node is serialized and needs RenderGraph to setup
  virtual void onLoad(RenderGraph &) {}
  // Runs the Node and writes to output pins
  virtual void run(RenderGraph &) {}

  template <class Archive> void serialize(Archive &ar) { ar(id, pos); }
};

// RenderGraph
struct RenderGraph {
  // Represents a pin of a node
  struct Pin {
    int id = -1;
    int node_id = -1;
    Data data;

    template <class Archive> void serialize(Archive &ar) {
      ar(VP(id), VP(node_id), VP(data));
    }
  };
  // Represents a connection between nodes
  struct Edge {
    int id = -1;
    int from = -1, to = -1;

    template <class Archive> void serialize(Archive &ar) {
      ar(VP(id), VP(from), VP(to));
    }
  };

private:
  std::map<int, std::shared_ptr<Node>> nodes = {};
  std::map<int, Edge> edges = {};
  std::map<int, Pin> pins = {};
  std::vector<int> run_order = {};

  int root_node = -1;
  bool should_stop = false;

public:
  std::shared_ptr<Assets<Shader>> shaders = nullptr;
  std::shared_ptr<Geometry> graph_geometry = nullptr;
  ImVec2 viewport_resolution = ImVec2(640, 480);

  RenderGraph(
      std::shared_ptr<Assets<Shader>> shaders =
          std::make_shared<Assets<Shader>>(),
      std::shared_ptr<Geometry> geo = std::make_shared<ScreenQuadGeometry>()) {
    this->shaders = shaders;
    this->graph_geometry = geo;
  };
  template <class Archive> void load(Archive &ar) {
    Data::Vec2 resolution;

    ar(                                         //
        VP(nodes),                              //
        VP(edges),                              //
        VP(pins),                               //
        VP(shaders),                            //
        VP(graph_geometry),                     //
        NVP("viewport_resolution", resolution), //
        VP(root_node)                           //
    );

    viewport_resolution.x = resolution[0];
    viewport_resolution.y = resolution[1];

    // Setup nodes
    for (auto &pair : nodes) {
      pair.second->onLoad(*this);
    }
  }
  template <class Archive> void save(Archive &ar) const {
    Data::Vec2 resolution = {viewport_resolution.x, viewport_resolution.y};

    for (auto &pair : nodes) { // Save node positions
      auto v = ImNodes::GetNodeGridSpacePos(pair.second->id);
      pair.second->pos = {v.x, v.y};
    }

    ar(                                         //
        VP(nodes),                              //
        VP(edges),                              //
        VP(pins),                               //
        VP(shaders),                            //
        VP(graph_geometry),                     //
        NVP("viewport_resolution", resolution), //
        VP(root_node)                           //
    );
  }
  // Sets up node position once an editor context is present
  // Used for loading only!
  void set_node_positions() {
    for (auto &pair : nodes) {
      ImVec2 pos = {pair.second->pos[0], pair.second->pos[1]};
      ImNodes::SetNodeGridSpacePos(pair.second->id, pos);
    }
  }
  void set_resolution(ImVec2 res) { viewport_resolution = res; }
  void set_geometry(std::shared_ptr<Geometry> geo) { graph_geometry = geo; }
  std::map<int, std::shared_ptr<Node>> &get_nodes() { return nodes; }
  std::map<int, Edge> &get_edges() { return edges; }
  std::map<int, Pin> &get_pins() { return pins; }
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
  int insert_root_node(std::shared_ptr<Node> node) {
    root_node = insert_node(node);
    return root_node;
  };
  int insert_node(std::shared_ptr<Node> node) {
    int nodeid = get_next_node_id();
    node->id = nodeid;
    node->onEnter(*this);
    nodes.insert(std::make_pair(node->id, node));
    return nodeid;
  };
  void register_pin(int nodeid, DataType type, int *pinid) {
    Pin pin = {id : get_next_pin_id(), node_id : nodeid, data : Data(type)};
    *pinid = pin.id;
    pins.insert(std::make_pair(pin.id, pin));
  };
  int insert_edge(int frompin, int topin) {
    if (pins.at(frompin).data.type != pins.at(topin).data.type) {
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
  /// Removes a node from the graph
  void delete_node(int nodeid) {
    nodes.at(nodeid)->onExit(*this);
    nodes.erase(nodes.find(nodeid));
  };
  /// Delete a pin from the graph including related edges
  void delete_pin(int pinid) {
    std::vector<int> marked;
    for (auto &pair : edges) {
      if (pair.second.from == pinid || pair.second.to == pinid) {
        marked.push_back(pair.second.id);
      }
    }
    for (auto &edgeid : marked)
      delete_edge(edgeid);
  };
  /// Delete an edge from the graph
  void delete_edge(int edgeid) { edges.erase(edges.find(edgeid)); };
  void render() {
    for (auto &pair : nodes) { // Render Nodes
      pair.second->render(*this);
    }
    for (auto &pair : edges) { // Render Edges
      ImNodes::Link(pair.first, pair.second.from, pair.second.to);
    }
  };
  Data get_pin_data(int pinid) { return pins.at(pinid).data; };
  void set_pin_data(int pinid, std::any ptr) {
    std::vector<int> connected_pins;
    for (auto &pair : edges) {
      if (pair.second.from == pinid)
        connected_pins.push_back(pair.second.to);
    }
    for (auto &cpin : connected_pins) {
      this->pins.at(cpin).data.set(ptr);
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

    for (auto &epair : edges) {
      for (auto &pin : pins) {
        // output pin connected to some other node
        if (pin == epair.second.to) {
          children.push_back(this->pins.at(epair.second.from).node_id);
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
  void stop() { should_stop = true; }
  void evaluate() {
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
  Node *get_node(int nodeid) { return nodes.at(nodeid).get(); }
  Node *get_root_node() { return nodes.at(root_node).get(); }
  // Prevents nodes from reusing previous data
  void clear_graph_data() {
    run_order.clear();
    for (auto &pin : pins) {
      pin.second.data.reset();
    }
  };
  // Default node layout
  void default_layout(std::shared_ptr<Shader>);
};
