#pragma once

#include "editor.h"
#include "graph.h"
#include "nodes.h"
#include "theme.h"
#include "utils.h"
#include <GL/gl.h> // For glGetTexImage otherwise GLES/gl3.h
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <memory>
#include <portable-file-dialogs.h>
#include <spdlog/spdlog.h>
#include <zep.h>

// A stateful widget
class Widget {
public:
  // Runs on the first frame when loaded
  virtual void onStartup() {};
  // Runs on shutdown or otherwise destroyed
  virtual void onShutdown() {};
  // Runs every frame
  virtual void onUpdate() {};
  // Runs every frame if visible
  virtual void render() {};
  template <class Archive> void serialize(Archive &ar) {}
};

class EditorWidget : public Widget {
private:
  std::shared_ptr<Shader> shader;
  bool is_dirty = false;
  uint64_t last_update = 0;

public:
  EditorWidget(std::shared_ptr<Shader> shader) : shader(shader) {}
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
    Zep::ZepEditor &zep = zep_get_editor();
    zep.InitWithText(shader->get_path().filename().string(),
                     shader->get_source());

    // zep.GetConfig().autoHideCommandRegion = false;
    zep.GetBuffers()[0]->SetFileFlags(Zep::FileFlags::SoftTabTwo, true);
    ZepStyleColorsCinder();

    last_update = zep.GetBuffers()[0]->GetUpdateCount();
  };
  void onShutdown() override { zep_destroy(); }
  void onUpdate() override {
    uint64_t new_update = zep_get_editor().GetBuffers()[0]->GetUpdateCount();
    if (new_update != last_update)
      this->is_dirty = true;
    last_update = new_update;

    if (is_dirty) {
      std::string text = get_buffer_text();
      shader->recompile_with_source(text);
      is_dirty = false;
    }

    if (isKeyJustPressed(ImGuiKey_F5)) {
      if (!shader->is_compiled())
        spdlog::error(shader->get_log());
      else
        shader->recompile();
    }
  };
  void render() override {
    zep_show(Zep::NVec2i(0, 0), shader->get_name().c_str());
  }
};
/// TODO:
/// Add option to display the image without stretching
class ViewportWidget : public Widget {
private:
  std::shared_ptr<RenderGraph> viewgraph;
  ImVec2 wsize = ImVec2(640, 480);

public:
  ViewportWidget(std::shared_ptr<RenderGraph> graph) { viewgraph = graph; }
  template <class Archive>
  static void load_and_construct(Archive &ar,
                                 cereal::construct<ViewportWidget> &construct) {
    std::shared_ptr<RenderGraph> viewgraph;
    ar(viewgraph);
    construct(viewgraph);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(viewgraph)); }
  void onStartup() override {};
  void onShutdown() override {}
  void onUpdate() override {};
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
class ConsoleWidget : public Widget {
private:
  constexpr static ImVec4 colors[] = {
      ImVec4(.4, .4, .4, .4),         // TRACE
      ImVec4(.302, .6314, .6627, 1),  // DEBUG
      ImVec4(.3608, .702, .2196, 1),  // INFO
      ImVec4(.9255, .9098, .3216, 1), // WARN
      ImVec4(.9765, .2196, .1529, 1), // ERROR
      ImVec4(.898, 1255, .1255, 1),   // CRITICAL
  };
  constexpr static size_t colors_len = sizeof(colors) / sizeof(colors[0]);

  bool sticky = true;
  bool colored = true;

public:
  template <class Archive> void serialize(Archive &ar) { ar(sticky); }
  void set_colored(bool col) { colored = col; }
  void render() override {
    ImGui::Begin("Console");

    if (colored) {
      auto buf = Global::instance().get_ringbuffer_sink()->last_raw();

      for (auto &msg : buf) {
        ImGui::TextUnformatted("[");
        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted(msg.logger_name.begin(), msg.logger_name.end());
        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted("] [");
        ImGui::SameLine(0, 0);

        if (msg.level < colors_len) {
          ImGui::PushStyleColor(ImGuiCol_Text, colors[msg.level]);
          ImGui::TextUnformatted(
              spdlog::level::to_string_view(msg.level).data());
          ImGui::PopStyleColor();
        } else
          ImGui::TextUnformatted(
              spdlog::level::to_string_view(msg.level).data());

        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted("] ");
        ImGui::SameLine(0, 0);

        // Manually add null terminator
        // TODO: Find a safer way that does not overwrite data
        const_cast<char *>(msg.payload.data())[msg.payload.size() - 1] = '\0';

        ImGui::TextWrapped(msg.payload.data());
      }
    } else {
      auto buf = Global::instance().get_ringbuffer_sink()->last_formatted();
      for (auto &msg : buf) {
        ImGui::TextWrapped(msg.data());
      }
    }

    float max_y = ImGui::GetScrollMaxY();
    float y = ImGui::GetScrollY();

    if (sticky)
      ImGui::SetScrollY(max_y);

    sticky = (y >= max_y);

    ImGui::End();
  }
};
class NodeEditorWidget : public Widget {
private:
  std::shared_ptr<RenderGraph> graph = nullptr;
  ImNodesEditorContext *context = ImNodes::EditorContextCreate();
  UndoContext history = UndoContext(50);

  // Start with a default type
  DataType current_link_type = DataType(0);
  bool add_nodes = false;
  bool focused = false;

public:
  NodeEditorWidget(std::shared_ptr<RenderGraph> graph) : graph(graph) {}
  template <class Archive>
  static void
  load_and_construct(Archive &ar,
                     cereal::construct<NodeEditorWidget> &construct) {
    std::shared_ptr<RenderGraph> graph;
    ar(graph);
    construct(graph);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(graph)); }
  void onStartup() override { graph->set_node_positions(context); };
  void onShutdown() override { ImNodes::EditorContextFree(context); }
  void onUpdate() override {
    auto io = ImGui::GetIO();

    { // ImNodes events
      int from_pin, to_pin;
      if (ImNodes::IsLinkCreated(&from_pin, &to_pin)) {
        if (graph->insert_edge(from_pin, to_pin) != -1)
          spdlog::info("Link created from {} to {}", from_pin, to_pin);
      }
      if (ImNodes::IsLinkStarted(&from_pin)) {
        current_link_type = graph->get_pin_data(from_pin).type;
      }
    }

    // Handle Input
    if (focused && !io.WantTextInput) {
      if (isKeyJustPressed(ImGuiKey_Z) && io.KeyCtrl && io.KeyShift)
        history.redo();
      else if (isKeyJustPressed(ImGuiKey_Z) && io.KeyCtrl)
        history.undo();
      if (isKeyJustPressed(ImGuiKey_A) && io.KeyShift) {
        add_nodes = true;
      }
      if (isKeyJustPressed(ImGuiKey_X)) {
        int length = ImNodes::NumSelectedNodes();
        if (length > 0) {
          int nodeids[length];
          ImNodes::GetSelectedNodes(nodeids);
          for (int id : nodeids) {
            graph->delete_node(id);
          }
        }
        length = ImNodes::NumSelectedLinks();
        if (length > 0) {
          int edgeids[length];
          ImNodes::GetSelectedLinks(edgeids);
          for (int id : edgeids) {
            graph->delete_edge(id);
          }
        }
      }
    }
  }
  void render() override {
    // Do not mutate EditorContext inside NodeEditor
    std::vector<std::shared_ptr<Node>> insert_nodes = {};

    ImGui::Begin("Node Editor");
    ImNodes::EditorContextSet(context);
    ImNodes::PushColorStyle(ImNodesCol_Link,
                            Data::COLORS_HOVER[current_link_type]);
    ImNodes::BeginNodeEditor();

    Global::setUndoContext(&history);
    graph.get()->render();

    { // Add Nodes
      if (add_nodes && !ImGui::IsPopupOpen("Add Nodes"))
        ImGui::OpenPopup("Add Nodes");
      else
        add_nodes = false;

      ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4);
      if (ImGui::BeginPopup("Add Nodes")) {
        ImGui::Dummy(ImVec2(140, 0)); // Min width
        ImGui::Indent(5);

        Data::Vec2 pos = {ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y};

        if (ImGui::Selectable("Fragment Shader"))
          insert_nodes.push_back(std::make_shared<FragmentShaderNode>());
        if (ImGui::Selectable("Time"))
          insert_nodes.push_back(std::make_shared<TimeNode>());
        if (ImGui::Selectable("Viewport"))
          insert_nodes.push_back(std::make_shared<ViewportNode>());
        if (ImGui::Selectable("Texture"))
          insert_nodes.push_back(std::make_shared<Texture2DNode>());

        if (ImGui::BeginMenu("Value Nodes")) {
          ImGui::Dummy(ImVec2(150, 0)); // Min width
          ImGui::Indent(5);
          if (ImGui::Selectable("Vec2"))
            graph->insert_node(std::make_shared<Vec2Node>());
          if (ImGui::Selectable("Float"))
            graph->insert_node(std::make_shared<FloatNode>());
          ImGui::Spacing();
          ImGui::EndMenu();
        }

        ImGui::Spacing();
        ImGui::Dummy(ImVec2(0, 60));
        ImGui::EndPopup();
      }
      ImGui::PopStyleVar();
    }

    focused = ImGui::IsWindowFocused();

    ImNodes::EndNodeEditor();
    ImNodes::PopColorStyle();
    ImGui::End();

    auto io = ImGui::GetIO();
    for (auto node : insert_nodes) {
      int id = graph->insert_node(node);
      ImNodes::SetNodeScreenSpacePos(id, {io.MousePos.x, io.MousePos.y});
    }
  }
};

#include <stb_image.h>
#include <stb_image_write.h>

/// Popup widget, should not be serialized
class ExportImagePopup : public Widget {
private:
  enum Format { PNG, JPEG };

  std::shared_ptr<RenderGraph> graph = nullptr;

  std::string export_path = "";
  int resolution[2] = {1920, 1080};
  int format = PNG;
  int quality = 90;

  float widget_width = 480;
  bool is_open = false;

public:
  ExportImagePopup() {}
  ExportImagePopup(std::shared_ptr<RenderGraph> graph) { this->graph = graph; }
  void open_popup() { is_open = true; }
  void set_graph(std::shared_ptr<RenderGraph> graph) { this->graph = graph; }
  void set_extension(const std::filesystem::path &ext) {
    std::filesystem::path path(export_path);
    path.replace_extension(ext);
    export_path = path.string();
  }
  void export_image() {
    graph->clear_graph_data();
    graph->set_resolution(ImVec2({float(resolution[0]), float(resolution[1])}));
    graph->evaluate();
    auto out = dynamic_cast<OutputNode *>(graph->get_root_node());
    GLuint img = out->get_image();

    unsigned char data[resolution[0] * resolution[1] * 4];
    glBindTexture(GL_TEXTURE_2D, img);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    bool success = false;

    stbi_flip_vertically_on_write(true);

    switch (format) {
    case PNG:
      success = stbi_write_png(export_path.c_str(), resolution[0],
                               resolution[1], 4, data, resolution[0] * 4);
      break;
    case JPEG:
      success = stbi_write_jpg(export_path.c_str(), resolution[0],
                               resolution[1], 4, data, quality);
      break;
    }

    if (success) {
      spdlog::info("Image Saved!");
    } else {
      spdlog::error("Failed to write image!");
    }
  }
  virtual void onStartup() override {
    // Default image path
    auto pwd = std::filesystem::current_path();
    switch (format) {
    case PNG:
      export_path = pwd / "image.png";
      break;
    case JPEG:
      export_path = pwd / "image.jpg";
      break;
    }
  }
  virtual void onShutdown() override { graph.reset(); }
  virtual void render() override {
    if (is_open && !ImGui::IsPopupOpen("Export Image"))
      ImGui::OpenPopup("Export Image");

    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 7));

    if (ImGui::BeginPopupModal("Export Image", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Default Graph Selected");

      ImGui::SetNextItemWidth(widget_width - ImGui::CalcTextSize("Path ...").x);
      ImGui::InputText("Path", &export_path, ImGuiInputTextFlags_ElideLeft);
      ImGui::SameLine();
      if (ImGui::Button("...")) {
        auto res = pfd::save_file("Export image to", export_path,
                                  {"Images", "*.png; *.jpg"})
                       .result();
        if (!res.empty())
          export_path = res;
      }

      ImGui::SetNextItemWidth(widget_width -
                              ImGui::CalcTextSize("Image Resolution").x);
      ImGui::InputInt2("Image Resolution", resolution);

      if (ImGui::RadioButton("PNG", &format, PNG))
        set_extension(".png");
      ImGui::SameLine();
      if (ImGui::RadioButton("JPEG", &format, JPEG))
        set_extension(".jpg");

      if (format == JPEG) {
        ImGui::SetNextItemWidth(widget_width -
                                ImGui::CalcTextSize("Image Quality").x);
        ImGui::DragInt("Image Quality", &quality, 1, 0, 100, "%d%%");
      }

      ImGui::BeginDisabled(!graph); // Disable if graph is NULL
      if (ImGui::Button("Export", ImVec2(widget_width / 2, 0))) {
        export_image();
        ImGui::CloseCurrentPopup();
        is_open = false;
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(widget_width / 2, 0))) {
        ImGui::CloseCurrentPopup();
        is_open = false;
      }

      ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
  }
};

// Type registration
#include <cereal/archives/json.hpp>

CEREAL_REGISTER_TYPE(EditorWidget)
CEREAL_REGISTER_TYPE(ViewportWidget)
CEREAL_REGISTER_TYPE(ConsoleWidget)
CEREAL_REGISTER_TYPE(NodeEditorWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, EditorWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, ViewportWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, ConsoleWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, NodeEditorWidget)
