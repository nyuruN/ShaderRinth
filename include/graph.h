#include <any>
#include <map>
#include <vector>

struct RenderGraph;

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

// Nodes:
// This base class defines function required for
// integration within the RenderGraph
class Node {
  int id;
  DataType type;
  std::any data;
  bool ready;

  // Executes the Node, recursively solving it if needed
  void run(RenderGraph *, DataType *, std::any *);
  // Renders the Node
  void render();
};

// Placeholder Node for Edges in a graph
class PinNode : Node {
  int id;
};

// RenderGraph
struct RenderGraph {
  // Represents a connection between nodes
  struct Edge {
    int from, to;
  };
  std::map<int, Node> nodes;
  std::map<int, Edge> edges;
  int root_node;

  void render();
  void eval();
  int edges_from_node();
  int insert_node(Node);
  int insert_edge(Edge);
};
