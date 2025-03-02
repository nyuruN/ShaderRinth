#pragma once

#include "data.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <toml++/toml.hpp>

#define BEGIN_INPUT_PIN(id, type)                                                                  \
  ImNodes::PushColorStyle(ImNodesCol_Pin, Data::COLORS[type]);                                     \
  ImNodes::PushColorStyle(ImNodesCol_PinHovered, Data::COLORS_HOVER[type]);                        \
  ImNodes::BeginInputAttribute(id);                                                                \
  ImNodes::PopColorStyle();                                                                        \
  ImNodes::PopColorStyle();

#define END_INPUT_PIN() ImNodes::EndInputAttribute();

#define BEGIN_OUTPUT_PIN(id, type)                                                                 \
  ImNodes::PushColorStyle(ImNodesCol_Pin, Data::COLORS[type]);                                     \
  ImNodes::PushColorStyle(ImNodesCol_PinHovered, Data::COLORS_HOVER[type]);                        \
  ImNodes::BeginOutputAttribute(id);                                                               \
  ImNodes::PopColorStyle();                                                                        \
  ImNodes::PopColorStyle();

#define END_OUTPUT_PIN() ImNodes::EndOutputAttribute();

#define REGISTER_NODE_FACTORY(type)                                                                \
  namespace {                                                                                      \
  static bool reg_##type = (NodeFactory::register_factory(#type, type::load), true);               \
  }

// Forward declares
struct RenderGraph;
struct AssetManager;
class Node;

// Nodes should implement a factory function as follows:
//
// static std::shared_ptr<Node> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
//   // Deserialization code
// }
struct NodeFactory {
public:
  using Factory =
      std::function<std::shared_ptr<Node>(toml::table &, std::shared_ptr<AssetManager>)>;

  static void register_factory(std::string name, Factory factory) {
    instance().factories.insert({name, factory});
  }
  static Factory get(std::string name) { return instance().factories.at(name); }

private:
  std::unordered_map<std::string, Factory> factories;

  static NodeFactory &instance() {
    static NodeFactory factory;
    return factory;
  }
};

// Nodes:
// This base class defines function required for
// integration within the RenderGraph
class Node {
public:
  int id = -1;
  Data::Vec2 pos = {};

  // Creates a copy of the object to be inserted in a graph
  //
  // Example implementation:
  // std::shared_ptr<Node> clone() const override {
  //   return std::make_shared<DerivedNode>(*this)
  // }
  virtual std::shared_ptr<Node> clone() const = 0;
  // Returns a layout of the node's attributes (pins)
  // Used for matching edges when copying
  //
  // Example implementation:
  // std::vector<int> layout() const override {
  //   return { my_output_pin };
  // }
  virtual std::vector<int> layout() const = 0;

  // Renders the Node
  virtual void render(RenderGraph &) {}
  // Called when the Node enters the graph for the first time
  virtual void onEnter(RenderGraph &) {}
  // Called when the Node is deleted from the graph
  virtual void onExit(RenderGraph &) {}
  // Called when the Node is serialized and needs to perform additional setup
  virtual void onLoad(RenderGraph &) {}
  // Runs the Node and writes to output pins
  virtual void run(RenderGraph &) {}

  static inline toml::table save(Data::Vec2 &pos) {
    toml::table t{
        {"x", pos[0]},
        {"y", pos[1]},
    };
    t.is_inline(true);
    return t;
  }
  static inline Data::Vec2 load_pos(toml::table &tbl) {
    Data::Vec2 pos;
    pos[0] = tbl["x"].value<float>().value();
    pos[1] = tbl["y"].value<float>().value();
    return pos;
  }
  virtual toml::table save() = 0;
  static std::shared_ptr<Node> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    std::string type = tbl["type"].value<std::string>().value();
    return NodeFactory::get(type)(tbl, assets);
  };
};
