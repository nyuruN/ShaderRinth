// [WIP] Widgets system
//
// Separates different parts of the application
#include "editor.h"
#include "graph.h"
#include "utils.h"
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <memory>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <zep.h>

// A stateful widget
class Widget {
public:
  // Runs on the FIRST startup frame
  virtual void onStartup() {};
  virtual void onShutdown() {};
  // Separate render code from state code
  virtual void onUpdate() {};
  virtual void render() {};
  template <class Archive> void serialize(Archive &ar) {}
};

class EditorWidget : public Widget {
private:
  std::shared_ptr<Shader> shader;

public:
  bool is_dirty = false;
  uint64_t last_update = 0;

  EditorWidget(std::shared_ptr<Shader> shader) { this->shader = shader; }
  template <class Archive>
  static void load_and_construct(Archive &ar,
                                 cereal::construct<EditorWidget> &construct) {
    std::shared_ptr<Shader> shader;
    ar(shader);
    construct(shader);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(shader)); }
  std::string get_buffer_text() {
    auto buffer = zep_get_editor().GetBuffers()[0];
    return buffer->GetBufferText(buffer->Begin(), buffer->End());
  }
  void onStartup() override {
    zep_init(Zep::NVec2f(1.0f, 1.0f));
    auto path = shader->get_path();
    zep_get_editor().InitWithText(path.filename().string(),
                                  shader->get_source());
    last_update = zep_get_editor().GetBuffers()[0]->GetUpdateCount();
  };
  void onShutdown() override { zep_destroy(); }
  void onUpdate() override {
    uint64_t new_update = zep_get_editor().GetBuffers()[0]->GetUpdateCount();
    if (new_update != last_update)
      this->is_dirty = true;
    last_update = new_update;

    if (is_dirty) {
      std::string text = get_buffer_text();
      shader->set_source(text);
      shader->should_recompile();
      is_dirty = false;
    }
  };
  void render() override {
    zep_show(Zep::NVec2i(0, 0), shader->get_name().c_str());
  }
};
/// TODO:
/// Add option to display the image without stretching
class ViewportWidget : public Widget {
public:
  std::shared_ptr<RenderGraph> viewgraph;
  ImVec2 wsize = ImVec2(640, 480);

  ViewportWidget(std::shared_ptr<RenderGraph> graph) { viewgraph = graph; }
  template <class Archive>
  static void load_and_construct(Archive &ar,
                                 cereal::construct<ViewportWidget> &construct) {
    std::shared_ptr<RenderGraph> viewgraph;
    ar(viewgraph);
    construct(viewgraph);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(viewgraph)); }
  void onStartup() override {
    // TODO
  };
  void onShutdown() override {
    // TODO
  }
  void onUpdate() override {
    // TODO
  };
  void render() override {
    ImGui::Begin("Viewport");
    ImGui::BeginChild("ViewportRender");

    wsize = ImGui::GetWindowSize();

    viewgraph->clear_graph_data();
    viewgraph->set_resolution(wsize);
    viewgraph->evaluate();
    auto out = dynamic_cast<OutputNode *>(viewgraph->get_root_node());
    GLuint output = out->get_image();

    ImGui::Image((ImTextureID)output, wsize, ImVec2(0, 1), ImVec2(1, 0));

    ImGui::EndChild();
    ImGui::End();
  }
};
/// TODO:
/// Add custom colors to console output
class ConsoleWidget : public Widget {
private:
  std::ostringstream log_stream;
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink;
  std::shared_ptr<spdlog::sinks::ostream_sink_mt> ostream_sink;
  bool sticky = false;

public:
  template <class Archive> void serialize(Archive &ar) {}
  std::shared_ptr<spdlog::logger> create_logger(char *name) {
    std::vector<spdlog::sink_ptr> sinks = {ostream_sink, console_sink};
    auto logger =
        std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    return logger;
  }
  void onStartup() override {
    console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(log_stream);
    ostream_sink->set_level(spdlog::level::info);

    auto logger = create_logger("Global");
    spdlog::set_default_logger(logger);
  }
  void onShutdown() override {
    // TODO
  }
  void onUpdate() override {
    // TODO
  }
  void render() override {
    ImGui::Begin("Console");

    ImGui::TextWrapped("%s", this->log_stream.str().c_str());

    float max_y = ImGui::GetScrollMaxY();
    float y = ImGui::GetScrollY();

    if (sticky)
      ImGui::SetScrollY(max_y);

    sticky = (y >= max_y);

    ImGui::End();
  }
};
class NodeEditorWidget : public Widget {
public:
  std::shared_ptr<RenderGraph> graph;

  NodeEditorWidget(std::shared_ptr<RenderGraph> graph) { this->graph = graph; }
  template <class Archive>
  static void
  load_and_construct(Archive &ar,
                     cereal::construct<NodeEditorWidget> &construct) {
    std::shared_ptr<RenderGraph> graph;
    ar(graph);
    construct(graph);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(graph)); }
  void onStartup() override {
    ImNodes::CreateContext();

    int out = graph->insert_root_node(std::make_unique<OutputNode>());
    int time = graph->insert_node(std::make_unique<TimeNode>());
    int vec = graph->insert_node(std::make_unique<Vec2Node>());
    int frag = graph->insert_node(std::make_unique<FragmentShaderNode>());

    auto f = dynamic_cast<FragmentShaderNode *>(graph->get_node(frag));
    auto v = dynamic_cast<Vec2Node *>(graph->get_node(vec));

    int out_in =
        dynamic_cast<OutputNode *>(graph->get_node(out))->get_input_pin();
    int time_out =
        dynamic_cast<TimeNode *>(graph->get_node(time))->get_output_pin();
    int frag_out = f->get_output_pin();
    int vec_out = v->get_output_pin();
    f->set_shader(graph->shaders->at("Default"));

    int u_res_in = f->add_uniform_pin(*graph, DataType::Vec2,
                                      (std::string) "u_resolution");
    int u_time_in =
        f->add_uniform_pin(*graph, DataType::Float, (std::string) "u_time");
    v->set_value(640.0f, 480.0f);

    graph->insert_edge(frag_out, out_in);
    graph->insert_edge(time_out, u_time_in);
    graph->insert_edge(vec_out, u_res_in);

    ImNodes::SetNodeGridSpacePos(out, ImVec2(500, 40));
    ImNodes::SetNodeGridSpacePos(vec, ImVec2(20, 40));
    ImNodes::SetNodeGridSpacePos(time, ImVec2(20, 160));
    ImNodes::SetNodeGridSpacePos(frag, ImVec2(200, 60));
  };
  void onShutdown() override { ImNodes::DestroyContext(); }
  void onUpdate() override {
    int from_pin, to_pin;
    if (ImNodes::IsLinkCreated(&from_pin, &to_pin)) {
      graph->insert_edge(from_pin, to_pin);
      spdlog::info("Link created from {} to {}", from_pin, to_pin);
    }
  }
  void render() override {
    ImGui::Begin("Node Editor");
    graph.get()->render();
    ImGui::End();
  }
};

// Type registration
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/xml.hpp>

CEREAL_REGISTER_TYPE(EditorWidget)
CEREAL_REGISTER_TYPE(ViewportWidget)
CEREAL_REGISTER_TYPE(ConsoleWidget)
CEREAL_REGISTER_TYPE(NodeEditorWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, EditorWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, ViewportWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, ConsoleWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, NodeEditorWidget)
