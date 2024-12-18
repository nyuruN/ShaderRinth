// [WIP] Widgets system
//
// Separates different parts of the application
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <editor.h>
#include <graph.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <zep.h>

// A stateful widget
class Widget {
  // Runs on the FIRST startup frame
  virtual void onStartup() {};
  virtual void onShutdown() {};
  // Separate render code from state code
  virtual void onUpdate() {};
  virtual void render() {};
};

class EditorWidget : public Widget {
private:
  std::string _source;

public:
  std::string buffername;
  bool is_dirty;

  Zep::NVec2i size = Zep::NVec2i(640, 480);
  uint64_t last_update = 0;

  EditorWidget(std::string filename, std::string source) {
    buffername = filename;
    _source = source;
  }
  std::string get_text() {
    auto buffer = zep_get_editor().GetBuffers()[0];
    buffer->GetBufferText(buffer->Begin(), buffer->End());
  }

  void onStartup() override {
    zep_init(Zep::NVec2f(1.0f, 1.0f));
    zep_get_editor().InitWithText(buffername, _source);
  };
  void onShutdown() override { zep_destroy(); }
  void onUpdate() override {
    auto buffer = zep_get_editor().GetBuffers()[0];
    uint64_t new_update = buffer->GetUpdateCount();

    if (new_update != last_update) {
      this->is_dirty = true;
    }
    last_update = new_update;

    zep_update(); // cursor blink etc.
  };
  void render() override { zep_show(size); }
};
/// TODO:
/// Add option to display the image without stretching
class ViewportWidget : public Widget {
public:
  ImVec2 wsize = ImVec2(640, 480);
  GLuint image = -1;

  void set_image(GLuint textureid) { image = textureid; }
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

    ImVec2 wsize = ImGui::GetWindowSize();
    ImGui::Image((ImTextureID)image, wsize, ImVec2(0, 1), ImVec2(1, 0));

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
  void onStartup() override {
    ImNodes::CreateContext();

    int out = graph->insert_root_node(std::make_unique<OutputNode>());
    int node = graph->insert_node(std::make_unique<TimeNode>());
    int vec = graph->insert_node(std::make_unique<Vec2Node>());
    int frag = graph->insert_node(std::make_unique<FragmentShaderNode>());
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
    this->graph.get()->render();
    ImGui::End();
  }
};
