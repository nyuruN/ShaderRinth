#include "widgets/outliner_widget.h"

#include "assets.h"
#include "geometry.h"
#include "shader.h"
#include "texture.h"
#include <imgui_stdlib.h>

void OutlinerWidget::render(bool *p_open) {
  ImGui::SetNextWindowSize({400, 200}, ImGuiCond_FirstUseEver);
  ImGui::Begin(title.c_str(), p_open);

  ImGui::BeginTable("Scene", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH);

  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  static unsigned int editing = 0;
  static std::string input;

  // Textures
  if (ImGui::TreeNodeEx("Textures")) {
    for (auto &pair : *assets->textures) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      if (pair.first == editing) {
        ImGui::Indent(20);
        if (ImGui::InputText("##hidelabel", &input, ImGuiInputTextFlags_EnterReturnsTrue)) {
          editing = 0;
          pair.second->get_name() = input;
        }
        ImGui::Indent(-20);
      } else {
        ImGui::TreeNodeEx(pair.second->get_name().c_str(), ImGuiTreeNodeFlags_Leaf);
        ImGui::TreePop();
      }

      ImGui::TableNextColumn();
      ImGui::PushID(pair.first);
      if (ImGui::Button("Edit")) {
        editing = pair.first;
        input = pair.second->get_name();
      }
      ImGui::PopID();
    }
    ImGui::TreePop();
  }

  ImGui::TableNextRow();
  ImGui::TableNextColumn();

  // Shaders
  if (ImGui::TreeNodeEx("Shaders")) {
    for (auto &pair : *assets->shaders) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      if (pair.first == editing) {
        ImGui::Indent(20);
        if (ImGui::InputText("##hidelabel", &input, ImGuiInputTextFlags_EnterReturnsTrue)) {
          editing = 0;
          pair.second->get_name() = input;
        }
        ImGui::Indent(-20);
      } else {
        ImGui::TreeNodeEx(pair.second->get_name().c_str(), ImGuiTreeNodeFlags_Leaf);
        ImGui::TreePop();
      }

      ImGui::TableNextColumn();
      ImGui::PushID(pair.first);
      if (ImGui::Button("Edit")) {
        editing = pair.first;
        input = pair.second->get_name();
      }
      ImGui::PopID();
    }
    ImGui::TreePop();
  }

  ImGui::TableNextRow();
  ImGui::TableNextColumn();

  // Geometries
  if (ImGui::TreeNodeEx("Geometries")) {
    for (auto &pair : *assets->geometries) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      if (pair.first == editing) {
        ImGui::Indent(20);
        if (ImGui::InputText("##hidelabel", &input, ImGuiInputTextFlags_EnterReturnsTrue)) {
          editing = 0;
          pair.second->get_name() = input;
        }
        ImGui::Indent(-20);
      } else {
        ImGui::TreeNodeEx(pair.second->get_name().c_str(), ImGuiTreeNodeFlags_Leaf);
        ImGui::TreePop();
      }

      ImGui::TableNextColumn();
      ImGui::PushID(pair.first);
      if (ImGui::Button("Edit")) {
        editing = pair.first;
        input = pair.second->get_name();
      }
      ImGui::PopID();
    }
    ImGui::TreePop();
  }

  ImGui::EndTable();

  ImGui::End();
}
