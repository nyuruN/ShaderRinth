#pragma once

#include "data.h"
#include "utils.h"
#include <GLES3/gl3.h>
#include <any>
#include <map>
#include <memory>

#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

// Forward declares
class Node;
class Geometry;
struct Shader;
struct Texture;
struct ImNodesEditorContext;

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

// RenderGraph
struct RenderGraph {
private:
  std::map<int, std::shared_ptr<Node>> nodes = {};
  std::map<int, Edge> edges = {};
  std::map<int, Pin> pins = {};
  std::vector<int> run_order = {};

  int root_node = -1;
  bool should_stop = false;
  int next_node_id = 0;
  int next_edge_id = 0;
  int next_pin_id = 0;

  // Calls onLoad() on all nodes
  void setup_nodes_on_load();

public:
  std::shared_ptr<Assets<Shader>> shaders = nullptr;
  std::shared_ptr<Assets<Texture>> textures = nullptr;
  std::shared_ptr<Geometry> graph_geometry = nullptr;
  ImVec2 viewport_resolution = ImVec2(640, 480);

  RenderGraph(std::shared_ptr<Assets<Shader>> shaders =
                  std::make_shared<Assets<Shader>>(),
              std::shared_ptr<Assets<Texture>> textures =
                  std::make_shared<Assets<Texture>>(),
              std::shared_ptr<Geometry> geo = NULL) {
    this->shaders = shaders;
    this->textures = textures;
    this->graph_geometry = geo;
  };
  template <class Archive> void load(Archive &ar) {
    Data::Vec2 resolution;

    ar(                                         //
        VP(nodes),                              //
        VP(edges),                              //
        VP(pins),                               //
        VP(shaders),                            //
        VP(textures),                           //
        VP(graph_geometry),                     //
        NVP("viewport_resolution", resolution), //
        VP(root_node),                          //
        VP(next_node_id),                       //
        VP(next_edge_id),                       //
        VP(next_pin_id)                         //
    );

    viewport_resolution.x = resolution[0];
    viewport_resolution.y = resolution[1];

    // Setup nodes
    setup_nodes_on_load();
  }
  template <class Archive> void save(Archive &ar) const {
    Data::Vec2 resolution = {viewport_resolution.x, viewport_resolution.y};

    // Save position in nodes
    get_node_positions();

    ar(                                         //
        VP(nodes),                              //
        VP(edges),                              //
        VP(pins),                               //
        VP(shaders),                            //
        VP(textures),                           //
        VP(graph_geometry),                     //
        NVP("viewport_resolution", resolution), //
        VP(root_node),                          //
        VP(next_node_id),                       //
        VP(next_edge_id),                       //
        VP(next_pin_id)                         //
    );
  }
  // Sets up node position for ImNodesEditorContext
  void set_node_positions(ImNodesEditorContext *context);
  // Gets node position for serialization
  void get_node_positions() const;
  void set_resolution(ImVec2 res) { viewport_resolution = res; }
  void set_geometry(std::shared_ptr<Geometry> geo) { graph_geometry = geo; }
  std::map<int, std::shared_ptr<Node>> &get_nodes() { return nodes; }
  std::map<int, Edge> &get_edges() { return edges; }
  std::map<int, Pin> &get_pins() { return pins; }
  int get_next_node_id() { return next_node_id++; };
  int get_next_edge_id() { return next_edge_id++; };
  int get_next_pin_id() { return next_pin_id++; };
  int insert_root_node(std::shared_ptr<Node> node) {
    root_node = insert_node(node);
    return root_node;
  };
  int insert_node(std::shared_ptr<Node> node);
  void register_pin(int nodeid, DataType type, int *pinid);
  int insert_edge(int frompin, int topin);
  /// Removes a node from the graph
  void delete_node(int nodeid);
  /// Delete a pin from the graph including related edges
  void delete_pin(int pinid);
  /// Delete an edge from the graph
  void delete_edge(int edgeid) { edges.erase(edges.find(edgeid)); };
  void render();
  Data get_pin_data(int pinid);
  void set_pin_data(int pinid, std::any ptr);
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
  void evaluate();
  Node *get_root_node() { return nodes.at(root_node).get(); }
  Node *get_node(int nodeid) { return nodes.at(nodeid).get(); }
  Edge get_edge(int edgeid) { return edges.at(edgeid); }
  // Prevents nodes from reusing previous data
  void clear_graph_data();
  // Default node layout
  void default_layout(std::shared_ptr<Shader>);
};
