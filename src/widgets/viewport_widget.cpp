
#include "widgets/viewport_widget.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "IconsFontAwesome6.h"
#include "graph.h"
#include "nodes/output_node.h"

void ViewportWidget::render(bool *p_open) {
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
      if (ImGui::Button(ICON_FA_REPEAT))
        viewgraph->time = 0.0f;
      if (paused) {
        if (ImGui::Button(ICON_FA_PLAY))
          paused = false;
      } else {
        if (ImGui::Button(ICON_FA_PAUSE))
          paused = true;
      }

      ImGui::EndMenuBar();
    }
    ImGui::EndChild();

    ImGui::End();
  }
}
