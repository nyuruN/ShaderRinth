#include "widgets/export_image_popup.h"

#include "graph.h"
#include "nodes/output_node.h"
#include "portable-file-dialogs.h"
#include <GL/gl.h> // For glGetTexImage otherwise GLES/gl3.h
#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <stb_image_write.h>

void ExportImagePopup::export_image() {
  graph->clear_graph_data();
  graph->set_resolution(ImVec2({float(resolution[0]), float(resolution[1])}));
  if (override_time)
    graph->set_time(time);
  graph->evaluate();

  GLuint img = 0;
  if (auto if_node = graph->get_root_node()) {
    auto out = dynamic_cast<OutputNode *>(if_node.value());
    img = out->get_image();
  }

  // unsigned char data[resolution[0] * resolution[1] * 4];
  std::vector<unsigned char> data(resolution[0] * resolution[1] * 4);
  glBindTexture(GL_TEXTURE_2D, img);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

  bool success = false;

  stbi_flip_vertically_on_write(true);

  switch (format) {
  case PNG:
    success = stbi_write_png(export_path.c_str(), resolution[0], resolution[1], 4, data.data(),
                             resolution[0] * 4);
    break;
  case JPEG:
    success =
        stbi_write_jpg(export_path.c_str(), resolution[0], resolution[1], 4, data.data(), quality);
    break;
  }

  if (success) {
    spdlog::info("Image saved in {}", export_path);
  } else {
    spdlog::error("Failed to write image!");
  }
}
void ExportImagePopup::render(bool *) {
  update_popup("Export Image");

  // Always center this window when appearing
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 7));

  if (ImGui::BeginPopupModal("Export Image", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::Text("Default Graph Selected");

    ImGui::SetNextItemWidth(widget_width - ImGui::CalcTextSize("Path ...").x);
    ImGui::InputText("Path", &export_path, ImGuiInputTextFlags_ElideLeft);
    ImGui::SameLine();
    if (ImGui::Button("...")) {
      auto res =
          pfd::save_file("Export image to", export_path, {"Images", "*.png; *.jpg"}).result();
      if (!res.empty())
        export_path = res;
    }

    ImGui::SetNextItemWidth(widget_width - ImGui::CalcTextSize("Image Resolution").x);
    ImGui::InputInt2("Image Resolution", resolution);

    if (ImGui::Checkbox("Time Override", &override_time))
      time = graph->time;
    ImGui::SameLine();
    if (override_time) {
      ImGui::InputDouble("##hidelabel", &time, 0.0f, 0.0f, "%.2f");
    } else {
      ImGui::BeginDisabled(true);
      ImGui::InputDouble("##hidelabel", &graph->time, 0.0f, 0.0f, "%.2f");
      ImGui::EndDisabled();
    }

    if (ImGui::RadioButton("PNG", &format, PNG))
      set_extension(".png");
    ImGui::SameLine();
    if (ImGui::RadioButton("JPEG", &format, JPEG))
      set_extension(".jpg");

    if (format == JPEG) {
      ImGui::SetNextItemWidth(widget_width - ImGui::CalcTextSize("Image Quality").x);
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
