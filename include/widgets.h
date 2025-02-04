#pragma once

#include "editor.h"
#include "graph.h"
#include "nodes.h"
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
/// TODO:
/// Add custom colors to console output
class ConsoleWidget : public Widget {
private:
  bool sticky = false;

public:
  template <class Archive> void serialize(Archive &ar) { ar(sticky); }
  void onStartup() override {}
  void onShutdown() override {}
  void onUpdate() override {}
  void render() override {
    ImGui::Begin("Console");

    ImGui::TextWrapped("%s",
                       Global::instance().get_log_stream_string().c_str());

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
  ImNodesEditorContext *context = nullptr;
  UndoContext history = UndoContext(50);

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
  void onStartup() override {
    context = ImNodes::EditorContextCreate();
    ImNodes::EditorContextSet(context);
    graph->set_node_positions();
  };
  void onShutdown() override { ImNodes::EditorContextFree(context); }
  void onUpdate() override {
    int from_pin, to_pin;
    if (ImNodes::IsLinkCreated(&from_pin, &to_pin)) {
      if (graph->insert_edge(from_pin, to_pin) != -1)
        spdlog::info("Link created from {} to {}", from_pin, to_pin);
    }
  }
  void render() override {
    ImGui::Begin("Node Editor");
    ImNodes::EditorContextSet(context);
    ImNodes::BeginNodeEditor();

    Global::instance().set_undo_context(&history);
    graph.get()->render();

    { // Add Nodes
      ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4);
      if (ImGui::BeginPopup("Add Nodes")) {
        ImGui::Dummy(ImVec2(140, 0)); // Min width
        ImGui::Indent(5);

        if (ImGui::Selectable("Fragment Shader"))
          graph->insert_node(std::make_shared<FragmentShaderNode>());
        if (ImGui::Selectable("Time"))
          graph->insert_node(std::make_shared<TimeNode>());
        if (ImGui::Selectable("Viewport"))
          graph->insert_node(std::make_shared<ViewportNode>());
        if (ImGui::Selectable("Texture"))
          graph->insert_node(std::make_shared<Texture2DNode>());

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

    bool focused = ImGui::IsWindowFocused();
    auto io = ImGui::GetIO();
    if (focused && !io.WantTextInput) {
      if (isKeyJustPressed(ImGuiKey_A) && io.KeyShift) {
        ImGui::OpenPopup("Add Nodes");
      }
    }

    ImNodes::EndNodeEditor();

    // Handle Input
    if (focused && !io.WantTextInput) {
      if (isKeyJustPressed(ImGuiKey_Z) && io.KeyCtrl && io.KeyShift)
        history.redo();
      else if (isKeyJustPressed(ImGuiKey_Z) && io.KeyCtrl)
        history.undo();
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

    ImGui::End();
  }
};
/// Popup widget, should not be serialized
class ExportImagePopup : public Widget {
private:
  enum Format { PNG, JPEG };

  std::shared_ptr<RenderGraph> graph;

  int resolution[2] = {1920, 1080};
  int format = PNG;
  int quality = 90;

  bool is_open = false;

public:
  ExportImagePopup(std::shared_ptr<RenderGraph> graph) : graph(graph) {}
  void open_popup() { is_open = true; }
  virtual void render() override {
    if (is_open && !ImGui::IsPopupOpen("Export Image"))
      ImGui::OpenPopup("Export Image");

    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Export Image", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {

      ImGui::Text("Default Graph Selected");

      ImGui::InputInt2("Image Resolution", resolution);

      ImGui::RadioButton("PNG", &format, PNG);
      ImGui::SameLine();
      ImGui::RadioButton("JPEG", &format, JPEG);

      if (format == JPEG) {
        ImGui::DragInt("Image Quality", &quality, 1, 0, 100, "%d%%");
      }

      if (ImGui::Button("Export", ImVec2(120, 0))) {
        graph->clear_graph_data();
        graph->set_resolution(
            ImVec2({float(resolution[0]), float(resolution[1])}));
        graph->evaluate();
        auto out = dynamic_cast<OutputNode *>(graph->get_root_node());
        GLuint img = out->get_image();

        unsigned char data[resolution[0] * resolution[1] * 4];
        glBindTexture(GL_TEXTURE_2D, img);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        bool success = false;

        switch (format) {
        case PNG: {
          success = stbi_write_png("test.png", resolution[0], resolution[1], 4,
                                   data, resolution[0] * 4);
          break;
        }
        case JPEG: {
          success = stbi_write_jpg("test.jpg", resolution[0], resolution[1], 4,
                                   data, quality);
          break;
        }
        }

        ImGui::CloseCurrentPopup();
        is_open = false;
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
        is_open = false;
      }

      ImGui::EndPopup();
    }
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
