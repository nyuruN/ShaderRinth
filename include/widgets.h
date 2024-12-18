// [WIP] Widgets system
//
// Separates different parts of the application
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <editor.h>
// #include <geometry.h>
#include <graph.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
// #include <utils.h>
#include <zep.h>

class Widget {
  // Runs on the FIRST startup frame
  virtual void onStartup() {};
  virtual void onShutdown() {};
  // Separate render code from state code
  virtual void onUpdate() {};
  virtual void render() {};
};

class EditorWidget : public Widget {
public:
  std::string filename;
  std::string source;
  bool is_dirty;

  Zep::NVec2i size = Zep::NVec2i(640, 480);
  uint64_t last_update = 0;

  EditorWidget(std::string filename, std::string source) {
    this->filename = filename;
    this->source = source;
  }

  void onStartup() override {
    zep_init(Zep::NVec2f(1.0f, 1.0f));
    zep_get_editor().InitWithText(this->filename, this->source);
  };
  void onShutdown() override { zep_destroy(); }
  void onUpdate() override {
    auto buffer = zep_get_editor().GetBuffers()[0];
    uint64_t new_update = buffer->GetUpdateCount();

    if (new_update != last_update) {
      this->source = buffer->GetBufferText(buffer->Begin(), buffer->End());
      this->is_dirty = true;
    }

    last_update = new_update;

    zep_update(); // cursor blink etc.
  };
  void render() override { zep_show(size); }
};
class ViewportWidget : public Widget {
public:
  // ScreenQuadGeometry screen_geo = ScreenQuadGeometry();
  //  unsigned int viewport_fbo;
  //  unsigned int viewport_colorbuffer;
  //  unsigned int vert_shader;
  //  unsigned int frag_shader;
  //  unsigned int program;

  ImVec2 wsize = ImVec2(640, 480);
  int image_override = 0;

  void onStartup() override {
    // glGenFramebuffers(1, &this->viewport_fbo);
    // glGenTextures(1, &this->viewport_colorbuffer);
    //
    // glBindTexture(GL_TEXTURE_2D, this->viewport_colorbuffer);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wsize.x, wsize.y, 0, GL_RGB,
    //              GL_UNSIGNED_BYTE, NULL);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glBindTexture(GL_TEXTURE_2D, 0);
  };
  void onShutdown() override {
    // glDeleteShader(this->vert_shader);
    // glDeleteShader(this->frag_shader);
    // glDeleteProgram(this->program);
    // glDeleteTextures(1, &this->viewport_colorbuffer);
    // glDeleteFramebuffers(1, &this->viewport_fbo);
  }
  void onUpdate() override {
    // Update Uniforms
    // int u_time_location = glGetUniformLocation(this->program, "u_time");
    // glUniform1f(u_time_location, glfwGetTime());
    // int u_resolution_location =
    //     glGetUniformLocation(this->program, "u_resolution");
    // glUniform2f(u_resolution_location, wsize.x, wsize.y);
  };
  void onResize(ImVec2 newsize) {
    // this->wsize = newsize; // TODO
    //
    // // Resize colorbuffer
    // glBindTexture(GL_TEXTURE_2D, this->viewport_colorbuffer);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wsize.x, wsize.y, 0, GL_RGB,
    //              GL_UNSIGNED_BYTE, NULL);
    // glBindTexture(GL_TEXTURE_2D, 0);
    //
    // // Update resolution uniform
    // int u_resolution_location =
    //     glGetUniformLocation(this->program, "u_resolution");
    // glUniform2f(u_resolution_location, wsize.x, wsize.y);
  };
  void render() override {
    ImGui::Begin("Viewport");
    ImGui::BeginChild("ViewportRender");

    ImVec2 wsize = ImGui::GetWindowSize();
    //
    // if (this->wsize.x != wsize.x || this->wsize.y != wsize.y) {
    //   onResize(wsize);
    // }
    //
    // glBindFramebuffer(GL_FRAMEBUFFER, this->viewport_fbo);
    //
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
    // GL_TEXTURE_2D,
    //                        this->viewport_colorbuffer, 0);
    //
    // if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    // {
    //   spdlog::error("Framebuffer is not complete!");
    // }
    //
    // glViewport(0, 0, wsize.x, wsize.y);
    // glClearColor(0.15f, 0.20f, 0.25f, 1.00f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glUseProgram(this->program);

    // screen_geo.draw_geometry();

    ImGui::Image((ImTextureID)image_override, wsize, ImVec2(0, 1),
                 ImVec2(1, 0));

    // glBindFramebuffer(GL_FRAMEBUFFER, 0); // reset default framebuffer

    ImGui::EndChild();
    ImGui::End();
  }
};
class ConsoleWidget : public Widget {
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

    OutputNode out;
    graph->insert_root_node(std::make_unique<OutputNode>(out));
    TimeNode node;
    graph->insert_node(std::make_unique<TimeNode>(node));
    Vec2Node vec;
    graph->insert_node(std::make_unique<Vec2Node>(vec));
    FragmentShaderNode frag;
    graph->insert_node(std::make_unique<FragmentShaderNode>(frag));
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
