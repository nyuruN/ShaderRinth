#pragma once

#include "graph.h"
#include "nodes/output_node.h"
#include "widget.h"
#include <imgui.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

/// A viewport to evaluate a RenderGraph in real-time
class ViewportWidget : public Widget {
private:
  AssetId<RenderGraph> graph_id;

  std::shared_ptr<RenderGraph> viewgraph;
  std::string title;
  ImVec2 wsize = ImVec2(640, 480);
  double last_time = 0.0f;

  bool paused = false;

public:
  ViewportWidget(int id, std::shared_ptr<AssetManager> assets, AssetId<RenderGraph> graph_id) {
    this->id = id;
    title = fmt::format("Viewport##{}", id);
    viewgraph = assets->getRenderGraph(graph_id).value();
    this->graph_id = graph_id;
  }
  void render(bool *p_open) override {
    ImGui::SetNextWindowSize({400, 400}, ImGuiCond_FirstUseEver);
    ImGui::Begin(title.c_str(), p_open,
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::BeginChild("ViewportRender");

    ImVec2 new_wsize = ImGui::GetWindowSize();
    bool resized = (wsize.x != new_wsize.x || wsize.y != new_wsize.y);
    wsize = new_wsize;
    double current = glfwGetTime();
    double delta = current - last_time;
    last_time = current;

    if (!paused || resized) {
      viewgraph->clear_graph_data();
      viewgraph->set_resolution(wsize);
      if (!paused)
        viewgraph->time += delta;
      viewgraph->evaluate();
    }

    GLuint output = 0;
    if (auto if_node = viewgraph->get_root_node()) {
      auto out = dynamic_cast<OutputNode *>(if_node.value());
      output = out->get_image();
    }

    ImGui::Image((ImTextureID)output, wsize, ImVec2(0, 1), ImVec2(1, 0));

    ImGui::EndChild();

    {
      float px = ImGui::GetCursorPosX() + ImGui::GetWindowPos().x;
      float py = ImGui::GetCursorPosY() + ImGui::GetWindowPos().y - 21;

      ImGui::SetNextWindowPos({px, py});
      ImGui::SetNextWindowSize({wsize.x, 20});
      ImGui::BeginChild("Menubar", {ImGui::GetWindowSize().x, 20}, ImGuiChildFlags_None,
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar);

      if (ImGui::BeginMenuBar()) {
        ImGui::Text("Time: %.2f", viewgraph->time);
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
          viewgraph->time = 0.0f;
        if (paused) {
          if (ImGui::Button("Unpause"))
            paused = false;
        } else {
          if (ImGui::Button("Pause"))
            paused = true;
        }

        ImGui::EndMenuBar();
      }
      ImGui::EndChild();

      ImGui::End();
    }
  }

  toml::table save() override {
    return toml::table{
        {"type", "ViewportWidget"},
        {"widget_id", id},
        {"graph_id", graph_id},
    };
  }
  static std::shared_ptr<Widget> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    int id = tbl["widget_id"].value<int>().value();
    AssetId<RenderGraph> graph_id = tbl["graph_id"].value<int>().value();
    auto w = ViewportWidget(id, assets, graph_id);
    return std::make_shared<ViewportWidget>(w);
  }
};

REGISTER_WIDGET_FACTORY(ViewportWidget);
