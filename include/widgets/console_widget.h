#pragma once

#include "utils.h"
#include "widget.h"

// Forward declares
struct AssetManager;

class ConsoleWidget : public Widget {
private:
  constexpr static ImVec4 colors[] = {
      ImVec4(.4, .4, .4, .4),         // TRACE
      ImVec4(.302, .6314, .6627, 1),  // DEBUG
      ImVec4(.3608, .702, .2196, 1),  // INFO
      ImVec4(.9255, .9098, .3216, 1), // WARN
      ImVec4(.9765, .2196, .1529, 1), // ERROR
      ImVec4(.898, 1255, .1255, 1),   // CRITICAL
  };
  constexpr static size_t colors_len = sizeof(colors) / sizeof(colors[0]);

  std::string title;
  bool sticky = true;
  bool colored = true;

public:
  ConsoleWidget(int id) {
    this->id = id;
    title = fmt::format("Console##{}", id);
  }
  void set_colored(bool col) { colored = col; }
  void render(bool *p_open) override {
    ImGui::SetNextWindowSize({400, 200}, ImGuiCond_FirstUseEver);
    ImGui::Begin(title.c_str(), p_open);

    if (colored) {
      auto buf = Global::instance().get_ringbuffer_sink()->last_raw();

      for (auto &msg : buf) {
        ImGui::TextUnformatted("[");
        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted(msg.logger_name.begin(), msg.logger_name.end());
        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted("] [");
        ImGui::SameLine(0, 0);

        if (msg.level < colors_len) {
          ImGui::PushStyleColor(ImGuiCol_Text, colors[msg.level]);
          ImGui::TextUnformatted(spdlog::level::to_string_view(msg.level).data());
          ImGui::PopStyleColor();
        } else
          ImGui::TextUnformatted(spdlog::level::to_string_view(msg.level).data());

        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted("] ");
        ImGui::SameLine(0, 0);

        // Manually add null terminator
        // TODO: Find a safer way that does not overwrite data
        const_cast<char *>(msg.payload.data())[msg.payload.size() - 1] = '\0';

        ImGui::TextWrapped(msg.payload.data());
      }
    } else {
      auto buf = Global::instance().get_ringbuffer_sink()->last_formatted();
      for (auto &msg : buf) {
        ImGui::TextWrapped(msg.data());
      }
    }

    float max_y = ImGui::GetScrollMaxY();
    float y = ImGui::GetScrollY();

    if (sticky)
      ImGui::SetScrollY(max_y);

    sticky = (y >= max_y);

    ImGui::End();
  }

  toml::table save() {
    return toml::table{
        {"type", "ConsoleWidget"},
        {"widget_id", id},
    };
  }
  static ConsoleWidget load(toml::table &tbl, std::shared_ptr<AssetManager>) {
    int id = tbl["widget_id"].value<int>().value();
    auto w = ConsoleWidget(id);
    return w;
  }
};
