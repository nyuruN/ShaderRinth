// Sourced from:
// https://github.com/cmaughan/zep_imgui/blob/main/demo/include/editor.h

#pragma once

// Re-exports
#include "zep.h" // IWYU pragma: export

// Helpers to create zep editor
Zep::ZepEditor &zep_get_editor();
void zep_init(const Zep::NVec2f &pixelScale,
              std::function<void(std::shared_ptr<Zep::ZepMessage>)> callback);
void zep_update();
void zep_show();
void zep_destroy();
