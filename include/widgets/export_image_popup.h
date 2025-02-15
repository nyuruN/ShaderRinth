#pragma once

#include "graph.h"
#include "nodes/output_node.h"
#include "widget.h"
#include <GL/gl.h> // For glGetTexImage otherwise GLES/gl3.h
#include <imgui_stdlib.h>
#include <portable-file-dialogs.h>
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
      success = stbi_write_png(export_path.c_str(), resolution[0], resolution[1], 4, data,
                               resolution[0] * 4);
      break;
    case JPEG:
      success = stbi_write_jpg(export_path.c_str(), resolution[0], resolution[1], 4, data, quality);
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
  virtual void render(bool *p_open = NULL) override {
    if (is_open && !ImGui::IsPopupOpen("Export Image"))
      ImGui::OpenPopup("Export Image");

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
  toml::table save() { throw std::runtime_error("Popup widget should not be serialized!"); }
};
