#pragma once

#include "data.h"

#define BEGIN_INPUT_PIN(id, type)                                              \
  ImNodes::PushColorStyle(ImNodesCol_Pin, Data::COLORS[type]);                 \
  ImNodes::PushColorStyle(ImNodesCol_PinHovered, Data::COLORS_HOVER[type]);    \
  ImNodes::BeginInputAttribute(id);                                            \
  ImNodes::PopColorStyle();                                                    \
  ImNodes::PopColorStyle();

#define END_INPUT_PIN() ImNodes::EndInputAttribute();

#define BEGIN_OUTPUT_PIN(id, type)                                             \
  ImNodes::PushColorStyle(ImNodesCol_Pin, Data::COLORS[type]);                 \
  ImNodes::PushColorStyle(ImNodesCol_PinHovered, Data::COLORS_HOVER[type]);    \
  ImNodes::BeginOutputAttribute(id);                                           \
  ImNodes::PopColorStyle();                                                    \
  ImNodes::PopColorStyle();

#define END_OUTPUT_PIN() ImNodes::EndOutputAttribute();

// Forward declares
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
