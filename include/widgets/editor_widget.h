#pragma once

#include "widget.h"

#include "assets.h"
#include "utils.h"

// Forward declares
struct Shader;
namespace Zep {
class ZepTabWindow;
class ZepBuffer;
} // namespace Zep

class EditorWidget : public Widget {
private:
  std::shared_ptr<Shader> shader;
  AssetId<Shader> shader_id;

  std::string title;
  Zep::ZepBuffer *buffer;
  Zep::ZepTabWindow *tab;
  bool is_dirty = false;
  uint64_t last_update = 0;

public:
  EditorWidget(int id, std::shared_ptr<AssetManager> assets, AssetId<Shader> shader_id);
  std::string get_buffer_text();
  std::shared_ptr<Shader> get_shader() { return shader; }
  void render(bool *) override;
  void onUpdate() override;
  void onStartup() override;
  void onShutdown() override;

  toml::table save() {
    return toml::table{
        {"type", "EditorWidget"},
        {"widget_id", id},
        {"shader_id", shader_id},
    };
  }
  static EditorWidget load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    int id = tbl["widget_id"].value<int>().value();
    AssetId<Shader> shader_id = tbl["shader_id"].value<int>().value();
    auto w = EditorWidget(id, assets, shader_id);
    return w;
  }
};
