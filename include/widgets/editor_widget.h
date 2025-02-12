#pragma once

#include "editor.h"
#include "shader.h"
#include "theme.h"
#include "widget.h"

#include <cereal/types/polymorphic.hpp>

class EditorWidget : public Widget {
private:
  std::shared_ptr<Shader> shader;
  bool is_dirty = false;
  uint64_t last_update = 0;

public:
  EditorWidget(std::shared_ptr<Shader> shader) : shader(shader) {}
  std::string get_buffer_text() {
    auto buffer = zep_get_editor().GetBuffers()[0];
    return buffer->GetBufferText(buffer->Begin(), buffer->End());
  }
  void onStartup() override {
    zep_init(Zep::NVec2f(1.0f, 1.0f));
    Zep::ZepEditor &zep = zep_get_editor();
    zep.InitWithText(shader->get_path().filename().string(),
                     shader->get_source());

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
  void render(bool *) override {
    zep_show(Zep::NVec2i(0, 0), shader->get_name().c_str());
  }

  template <class Archive>
  static void load_and_construct(Archive &ar,
                                 cereal::construct<EditorWidget> &construct) {
    std::shared_ptr<Shader> shader;
    ar(shader);
    construct(shader);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(shader)); }
};

// Type registration
#include <cereal/archives/json.hpp>
CEREAL_REGISTER_TYPE(EditorWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, EditorWidget)
