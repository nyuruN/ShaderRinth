#include "widgets/outliner_widget.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "assets.h"
#include "events.h"
#include "geometry.h"
#include "shader.h"
#include "texture.h"
#include "widgets/editor_widget.h"

#include "IconsFontAwesome6.h"

void render_entry(AssetId<Asset> asset_id, std::string &name, AssetId<Asset> &editing,
                  std::string &input, std::vector<AssetId<Asset>> &deferred_delete) {
  ImGui::TableNextRow();
  ImGui::TableNextColumn();

  if (asset_id == editing) {
    ImGui::Indent(20);
    if (ImGui::InputText("##hidelabel", &input, ImGuiInputTextFlags_EnterReturnsTrue)) {
      editing = 0;
      name = input;
    }
    ImGui::Indent(-20);
  } else {
    ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf);
    ImGui::TreePop();
  }

  ImGui::TableNextColumn();
  if (ImGui::Button(ICON_FA_PENCIL)) {
    editing = asset_id;
    input = name;
  }
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_MINUS)) {
    deferred_delete.push_back(asset_id);
  }
  ImGui::SameLine();
  ImGui::Dummy({5, 0});
}
void OutlinerWidget::render(bool *p_open) {
  ImGui::SetNextWindowSize({400, 200}, ImGuiCond_FirstUseEver);
  if (!ImGui::Begin(title.c_str(), p_open)) {
    ImGui::End();
    return;
  }

  ImGui::BeginTable("Scene", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH);

  ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);
  ImGui::TableSetupColumn("Edit", ImGuiTableColumnFlags_WidthFixed);

  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  static unsigned int editing = 0;
  static std::string input;

  // Textures
  if (ImGui::TreeNodeEx("Textures")) {
    std::vector<AssetId<Texture>> deferred_delete = {};
    for (auto &pair : *assets->getTextureCollection()) {
      ImGui::PushID(pair.first);
      render_entry(pair.first, pair.second->get_name(), editing, input, deferred_delete);
      ImGui::PopID();
    }
    for (auto id : deferred_delete) {
      assets->getTextureCollection()->at(id)->destroy();
      assets->getTextureCollection()->erase(id);
    }
    ImGui::TreePop();
  }

  ImGui::TableNextRow();
  ImGui::TableNextColumn();

  bool shaders_open = ImGui::TreeNodeEx("Shaders");
  ImGui::TableNextColumn();
  if (ImGui::Button(ICON_FA_SQUARE_PLUS "##shader")) { // Handle creation
    AssetId<Shader> id = assets->insertShader(std::make_shared<Shader>(Shader("NewShader")));
    EventQueue::push(AddWidget(
        std::make_shared<EditorWidget>(EditorWidget(assets->get_widget_id(), assets, id))));
    spdlog::info("Shader \"NewShader\" created!");
  }
  // Shaders
  if (shaders_open) {
    std::vector<AssetId<Shader>> deferred_delete = {};
    for (auto &pair : *assets->getShaderCollection()) {
      ImGui::PushID(pair.first);
      render_entry(pair.first, pair.second->get_name(), editing, input, deferred_delete);
      ImGui::PopID();
    }
    for (auto id : deferred_delete) { // Handle deletion
      assets->getShaderCollection()->at(id)->destroy();
      assets->getShaderCollection()->erase(id);
    }
    ImGui::TreePop();
  }

  ImGui::TableNextRow();
  ImGui::TableNextColumn();

  // Geometries
  if (ImGui::TreeNodeEx("Geometries")) {
    std::vector<AssetId<Geometry>> deferred_delete = {};
    for (auto &pair : *assets->getGeometryCollection()) {
      ImGui::PushID(pair.first);
      ImGui::BeginDisabled(true);
      render_entry(pair.first, pair.second->get_name(), editing, input, deferred_delete);
      ImGui::EndDisabled();
      ImGui::PopID();
    }
    ImGui::TreePop();
  }

  ImGui::EndTable();

  ImGui::End();
}
