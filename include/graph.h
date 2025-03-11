#pragma once

#include "assets.h"
#include "data.h"
#include <any>
#include <imgui.h>
#include <map>
#include <memory>

#include <toml++/toml.hpp>

// Forward declares
class Node;
class Geometry;
struct Shader;
struct Texture;
struct ImNodesEditorContext;
struct Data;

// Represents a pin of a node
struct Pin {
  int id = -1;
  int node_id = -1;
  Data data;
};

// Represents a connection between nodes
struct Edge {
  int id = -1;
  int from = -1, to = -1;
};

std::shared_ptr<Node> load_node(toml::table &tbl, std::shared_ptr<AssetManager> assets);

// RenderGraph
struct RenderGraph : Asset {
private:
  std::map<int, std::shared_ptr<Node>> nodes = {};
  std::map<int, Edge> edges = {};
  std::map<int, Pin> pins = {};

  AssetId<Geometry> geometry_id;
  int root_node = -1;
  int next_node_id = 0;
  int next_edge_id = 0;
  int next_pin_id = 0;

  std::vector<int> run_order = {};
  bool should_stop = false;

  // Calls onLoad() on all nodes
  void setup_nodes_on_load();

public:
  ImVec2 viewport_resolution = ImVec2(640, 480);
  std::shared_ptr<Geometry> graph_geometry = nullptr;

  RenderGraph(std::shared_ptr<AssetManager> assets = std::make_shared<AssetManager>(),
              AssetId<Geometry> geometry_id = 0) {
    this->graph_geometry = assets->getGeometry(geometry_id).value();
    this->geometry_id = geometry_id;
  };
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
  // Starting from a root node, traverse the N-ary tree
  // to perform a topological sort
  void traverse(std::vector<int> &order, int nodeid) {
    order.push_back(nodeid);

    std::vector<int> children;
    get_children(nodeid, children);

    for (auto &child : children) {
      traverse(order, child);
    }
  }
  // Creates a topological run order of the DAG
  void topological_order() {
    if (!get_root_node())
      return;
    traverse(run_order, root_node);
  };
  void stop() { should_stop = true; }
  void evaluate();
  int get_root_node_id() { return root_node; }
  void set_root_node(int root_node) { this->root_node = root_node; }
  std::optional<Node *> get_root_node() {
    try {
      return nodes.at(root_node).get();
    } catch (std::out_of_range) {
      return {};
    }
  }
  Node *get_node(int nodeid) { return nodes.at(nodeid).get(); }
  Edge get_edge(int edgeid) { return edges.at(edgeid); }
  Pin get_pin(int pinid) { return pins.at(pinid); }
  // Prevents nodes from reusing previous data
  void clear_graph_data();
  // Default node layout
  void default_layout(std::shared_ptr<AssetManager> assets, AssetId<Shader> shader_id);
  toml::table save(std::filesystem::path project_root) override;
  // Attempts to load RenderGraph, throwing std::bad_optional_access if failed
  static std::shared_ptr<RenderGraph> load(toml::table &tbl, std::shared_ptr<AssetManager> assets);
};
