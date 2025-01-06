#include "imnodes/imnodes.h"
#include "material.h"
#include "utils.h"
#include <GLFW/glfw3.h>
#include <any>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cstdlib>
#include <imgui.h>
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
    Data data = graph.get_pin_data(input_pin);
    try {
      out_texture = data.get<GLuint>();
    } catch (std::bad_any_cast) {
      spdlog::error("Nothing is connected!");
    }
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(input_pin, out_texture);
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
    graph.set_pin_data(output_pin, (Data::Float)glfwGetTime());
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin);
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
    graph.set_pin_data(output_pin, (Data::Float)value);
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin, value);
  }
};
class Vec2Node : public Node {
  int output_pin;
  std::array<float, 2> value = {0};

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
      ImGui::InputFloat2("##hidelabel", value.data(), "%.1f");
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
    graph.set_pin_data(output_pin, (Data::Vec2)value);
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin, value);
  }
};
class FragmentShaderNode : public Node {
  struct UniformPin {
    int pinid;
    DataType type;
    std::string identifier;

    template <class Archive> void serialize(Archive &ar) {
      ar(pinid, type, identifier);
    }
  };

  int output_pin;
  std::vector<UniformPin> uniform_pins;
  std::shared_ptr<Shader> shader;

  GLuint image_fbo;
  GLuint image_colorbuffer;
  GLuint bound_textures = 0;

  const float node_width = 240.0f;

public:
  void set_shader(std::shared_ptr<Shader> shader) { this->shader = shader; }
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

    if (ImGui::BeginCombo("##hidelabel", "preview")) {
      auto shadermap = graph.shaders;
      for (auto const &pair : *shadermap) {
        bool is_selected =
            (shader) && (shader->get_name() == pair.second->get_name());
        if (ImGui::Selectable(pair.first.c_str(), is_selected))
          set_shader(pair.second);
        // Set the initial focus when opening the combo (scrolling + keyboard
        // navigation focus)
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    {
      ImNodes::BeginOutputAttribute(output_pin);
      ImGui::Indent(node_width - ImGui::CalcTextSize("Image").x);
      ImGui::Text("Image");
      ImNodes::EndOutputAttribute();
    }

    if (ImGui::BeginPopup("add_uniform_popup")) { // Creation logic
      ImGui::SeparatorText("Uniforms");
      for (auto &type : Data::ALL) {
        if (ImGui::MenuItem(Data::type_name(type))) {
          add_uniform_pin(graph, type,
                          fmt::format("u_{}", Data::type_name(type)));
        }
      }
      ImGui::EndPopup();
    }

    { // Render uniforms + delete logic
      std::vector<int> marked;
      int idx = 0;
      for (auto &pin : uniform_pins) {
        ImNodes::BeginInputAttribute(pin.pinid);
        ImGui::Text(Data::type_name(pin.type));
        ImGui::SameLine();

        // TODO: I should optimize this someday :(
        std::vector<char> buf(pin.identifier.begin(), pin.identifier.end());
        buf.push_back('\0');
        ImGui::SetNextItemWidth(
            node_width - 20 - ImGui::CalcTextSize(" - ").x -
            ImGui::CalcTextSize(Data::type_name(pin.type)).x);
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
    onLoad(graph);
  }
  void onLoad(RenderGraph &graph) override {
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
    if (!shader) {
      graph.should_stop = true;
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
      Data data = graph.get_pin_data(pin.pinid);
      set_uniform(*shader.get(), pin.identifier, data);
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

    graph.set_pin_data(output_pin, (Data::Texture2D)image_colorbuffer);
  }
  unsigned int get_next_texture_unit() {
    unsigned int unit = bound_textures;
    bound_textures++;
    return GL_TEXTURE0 + unit;
  }
  void set_uniform(Shader &shader, std::string identifier, Data data) {
    GLuint loc = shader.get_uniform_loc(identifier.data());
    // clang-format off
    switch (data.type) {
    case DataType::Int: {
      auto val = data.get<int>(); glUniform1iv(loc, 1, &val); break;
    };
    case DataType::IVec2: {
      auto val = data.get<Data::IVec2>().data(); glUniform2iv(loc, 1, val); break;
    };
    case DataType::IVec3: {
      auto val = data.get<Data::IVec3>().data(); glUniform3iv(loc, 1, val); break;
    };
    case DataType::IVec4: {
      auto val = data.get<Data::IVec4>().data(); glUniform4iv(loc, 1, val); break;
    };
    case DataType::UInt: {
      auto val = data.get<Data::UInt>(); glUniform1uiv(loc, 1, &val); break;
    };
    case DataType::UVec2: {
      auto val = data.get<Data::UVec2>().data(); glUniform2uiv(loc, 1, val); break;
    };
    case DataType::UVec3: {
      auto val = data.get<Data::UVec3>().data(); glUniform3uiv(loc, 1, val); break;
    };
    case DataType::UVec4: {
      auto val = data.get<Data::UVec4>().data(); glUniform4uiv(loc, 1, val); break;
    };
    case DataType::Float: {
      auto val = data.get<Data::Float>(); glUniform1fv(loc, 1, &val); break;
    };
    case DataType::Vec2: {
      auto val = data.get<Data::Vec2>().data(); glUniform2fv(loc, 1, val); break;
    };
    case DataType::Vec3: {
      auto val = data.get<Data::Vec3>().data(); glUniform3fv(loc, 1, val); break;
    };
    case DataType::Vec4: {
      auto val = data.get<Data::Vec4>().data(); glUniform4fv(loc, 1, val); break;
    };
    case DataType::Texture2D: {
      GLuint texture = data.get<Data::Texture2D>();
      int unit = get_next_texture_unit();
      glActiveTexture(unit);
      glBindTexture(GL_TEXTURE_2D, texture);
      glUniform1i(loc, texture);
      break;
    };
    }
    // clang-format on
  }
  template <class Archive> void serialize(Archive &ar) {
    ar(cereal::base_class<Node>(this));
    ar(output_pin, uniform_pins, shader);
  }
};

// Type registration
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/xml.hpp>

CEREAL_REGISTER_TYPE(OutputNode)
CEREAL_REGISTER_TYPE(TimeNode)
CEREAL_REGISTER_TYPE(FloatNode)
CEREAL_REGISTER_TYPE(Vec2Node)
CEREAL_REGISTER_TYPE(FragmentShaderNode)
