#pragma once

#include "widget.h"

#include <fmt/format.h>

// Forward declares
struct AssetManager;

class OutlinerWidget : public Widget {
private:
  std::string title;
  std::shared_ptr<AssetManager> assets;

public:
  OutlinerWidget(int id, std::shared_ptr<AssetManager> assets) {
    this->id = id;
    this->assets = assets;
    title = fmt::format("Outliner##{}", id);
  }
  void render(bool *p_open) override;

  toml::table save() {
    return toml::table{
        {"type", "OutlinerWidget"},
        {"widget_id", id},
    };
  }
  static std::shared_ptr<Widget> load(toml::table &tbl, std::shared_ptr<AssetManager> assets) {
    int id = tbl["widget_id"].value<int>().value();
    auto w = OutlinerWidget(id, assets);
    return std::make_shared<OutlinerWidget>(w);
  }
};

REGISTER_WIDGET_FACTORY(OutlinerWidget);
