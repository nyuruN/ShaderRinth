#pragma once

#include "widget.h"

#include "utils.h"
#include <cereal/types/polymorphic.hpp>

// Forward declares
struct Shader;
namespace Zep {
class ZepTabWindow;
class ZepBuffer;
} // namespace Zep

class EditorWidget : public Widget {
private:
  std::shared_ptr<Shader> shader;

  std::string title;
  Zep::ZepBuffer *buffer;
  Zep::ZepTabWindow *tab;
  bool is_dirty = false;
  uint64_t last_update = 0;

public:
  EditorWidget(int id, std::shared_ptr<Shader> shader);
  std::string get_buffer_text();
  std::shared_ptr<Shader> get_shader() { return shader; }
  void render(bool *) override;
  void onUpdate() override;
  void onStartup() override;
  void onShutdown() override;

  template <class Archive>
  static void load_and_construct(Archive &ar, cereal::construct<EditorWidget> &construct) {
    std::shared_ptr<Shader> shader;
    int id;
    ar(id, shader);
    construct(id, shader);
  }
  template <class Archive> void serialize(Archive &ar) { ar(VP(id), VP(shader)); }
};

// Type registration
#include <cereal/archives/json.hpp>
CEREAL_REGISTER_TYPE(EditorWidget)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Widget, EditorWidget)
