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

  template <typename T> T get() { return std::any_cast<T>(data); }
  void set(std::any d) { data = d; }
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
  int id;

  // Renders the Node
  virtual void render(RenderGraph &) {}
  virtual void onEnter(RenderGraph &) {}
  virtual void onExit(RenderGraph &) {}
  // Called when the Node is serialized and needs RenderGraph to setup
  virtual void onLoad(RenderGraph &) {}
  // Runs the Node and writes to output pins
  virtual void run(RenderGraph &) {}

  template <class Archive> void serialize(Archive &ar) { ar(id); }
};

// RenderGraph
struct RenderGraph {
  // Represents a pin of a node
  struct Pin {
    int id;
    int node_id;
    Data data;

    template <class Archive> void serialize(Archive &ar) {
      ar(CEREAL_NVP(id), CEREAL_NVP(node_id), CEREAL_NVP(data));
    }
  };
  // Represents a connection between nodes
  struct Edge {
    int id;
    int from, to;

    template <class Archive> void serialize(Archive &ar) {
      ar(CEREAL_NVP(id), CEREAL_NVP(from), CEREAL_NVP(to));
    }
  };
  std::map<int, std::unique_ptr<Node>> nodes;
  std::map<int, Edge> edges;
  std::map<int, Pin> pins;

  std::shared_ptr<Assets<Shader>> shaders;
  std::shared_ptr<Geometry> graph_geometry;
  ImVec2 viewport_resolution = ImVec2(640, 480);
  int root_node;

  std::vector<int> run_order;
  bool should_stop = false;
  ImNodesContext *context;

  RenderGraph(
      std::shared_ptr<Assets<Shader>> shaders =
          std::make_shared<Assets<Shader>>(),
      std::shared_ptr<Geometry> geo = std::make_shared<ScreenQuadGeometry>()) {
    this->shaders = shaders;
    this->graph_geometry = geo;
    this->context = ImNodes::CreateContext();
  };
  template <class Archive> void load(Archive &ar) {
    Data::Vec2 resolution;
    ar(                                                      //
        CEREAL_NVP(nodes),                                   //
        CEREAL_NVP(edges),                                   //
        CEREAL_NVP(pins),                                    //
        CEREAL_NVP(shaders),                                 //
        CEREAL_NVP(graph_geometry),                          //
        cereal::make_nvp("viewport_resolution", resolution), //
        CEREAL_NVP(root_node)                                //
    );

    viewport_resolution.x = resolution[0];
    viewport_resolution.y = resolution[1];

    // Setup nodes
    for (auto &pair : nodes) {
      pair.second->onLoad(*this);
    }
  }
  template <class Archive> void save(Archive &ar) const {
    auto resolution =
        Data::Vec2({viewport_resolution.x, viewport_resolution.y});
    ar(                                                      //
        CEREAL_NVP(nodes),                                   //
        CEREAL_NVP(edges),                                   //
        CEREAL_NVP(pins),                                    //
        CEREAL_NVP(shaders),                                 //
        CEREAL_NVP(graph_geometry),                          //
        cereal::make_nvp("viewport_resolution", resolution), //
        CEREAL_NVP(root_node)                                //
    );
  }
  void destroy() { ImNodes::DestroyContext(context); }
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
    int nodeid = insert_node(std::move(node));
    root_node = nodeid;
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
      pin.second.data.set(nullptr);
    }
  };
  void default_layout();
};
