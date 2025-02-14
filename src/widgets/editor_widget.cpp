#include "widgets/editor_widget.h"

#include "editor.h"
#include "shader.h"

EditorWidget::EditorWidget(int id, std::shared_ptr<Shader> shader) : shader(shader) {
  this->id = id;
  title = fmt::format("{}##{}", shader->get_name().c_str(), id);
}
std::string EditorWidget::get_buffer_text() {
  return buffer->GetBufferText(buffer->Begin(), buffer->End());
}
void EditorWidget::onStartup() {
  auto &zep = zep_get_editor();

  buffer = zep.GetEmptyBuffer(shader->get_path().filename());
  buffer->SetText(shader->get_source());
  buffer->SetFileFlags(Zep::FileFlags::SoftTabTwo, true);

  tab = zep.AddTabWindow();

  // NOTE: A tab should only ever have ONE window
  tab->AddWindow(buffer);

  last_update = buffer->GetUpdateCount();
};
void EditorWidget::onShutdown() {
  auto &zep = zep_get_editor();

  zep.RemoveTabWindow(tab);
  zep.RemoveBuffer(buffer);
}
void EditorWidget::onUpdate() {
  uint64_t new_update = buffer->GetUpdateCount();
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
void EditorWidget::render(bool *p_open) {
  ImGui::SetNextWindowSize({600, 400}, ImGuiCond_FirstUseEver);
  if (!ImGui::Begin(title.c_str(), p_open, ImGuiWindowFlags_NoScrollbar)) {
    ImGui::End();
    return;
  }

  zep_get_editor().SetCurrentTabWindowUnchecked(tab);
  zep_show();

  ImGui::End();
}
