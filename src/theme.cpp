#include "theme.h"
#include "editor.h"
#include <zep.h>

namespace ImGui {
// Sourced from: https://github.com/GraphicsProgramming/dear-imgui-styles
void StyleColorsCinder() {
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowMinSize = ImVec2(160, 20);
  style.FramePadding = ImVec2(4, 2);
  style.ItemSpacing = ImVec2(6, 2);
  style.ItemInnerSpacing = ImVec2(6, 4);
  style.Alpha = 0.95f;
  style.WindowRounding = 4.0f;
  style.FrameRounding = 2.0f;
  style.IndentSpacing = 6.0f;
  style.ItemInnerSpacing = ImVec2(2, 4);
  style.ColumnsMinSpacing = 50.0f;
  style.GrabMinSize = 14.0f;
  style.GrabRounding = 16.0f;
  style.ScrollbarSize = 12.0f;
  style.ScrollbarRounding = 16.0f;

  style.Colors[ImGuiCol_Text] = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
  style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
  style.Colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
  style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
  // style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f * 1.1, 0.22f * 1.1, 0.27f * 1.1, 1.00f);

  style.Colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
  style.Colors[ImGuiCol_TabSelected] = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
  style.Colors[ImGuiCol_TabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_TabDimmed] = ImVec4(0.20f * 1.1, 0.22f * 1.1, 0.27f * 1.1, 1.00f);
  style.Colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.20f * 1.1, 0.22f * 1.1, 0.27f * 1.1, 1.00f);
  style.Colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
  style.Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);

  style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
  style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
  style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
  style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
  style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
  style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
  style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
  // style.Colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.9f);
  style.Colors[ImGuiCol_PopupBg] = ImVec4(0.20f * .9, 0.22f * .9, 0.27f * .9, 0.9f);
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f * .8, 0.22f * .8, 0.27f * .8, 0.76f);
  style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.47f, 0.77f, 0.83f, 0.2f);
}
} // namespace ImGui

void ZepStyleColorsCinder() {
  using namespace Zep;

  Zep::ZepTheme &zep = zep_get_editor().GetTheme();

  // clang-format off
  zep.SetColor(ThemeColor::AirlineBackground, NVec4f(0.20, 0.2157, 0.2627, 1));
  zep.SetColor(ThemeColor::CommandLineBackground, NVec4f(0.20*.75, 0.2157*.75, 0.2627*.75, 1));
  zep.SetColor(ThemeColor::Background, NVec4f(0.20, 0.2157, 0.2627, 1));
  zep.SetColor(ThemeColor::Comment, NVec4f(.3608, .3882, .4392, 1));
  //zep.SetColor(ThemeColor::CursorInsert, NVec4f(1.0f, 1.0f, 1.0f, .9f));
  zep.SetColor(ThemeColor::CursorLineBackground, NVec4f(0.20*1.15, 0.2157*1.15, 0.2627*1.15, 1));
  //zep.SetColor(ThemeColor::CursorNormal, NVec4f(130.0f / 255.0f, 140.0f / 255.0f, 230.0f / 255.0f, 1.0f));
  //zep.SetColor(ThemeColor::Dark, NVec4f(0.0f, 0.0f, 0.0f, 1.0f));
  //zep.SetColor(ThemeColor::Error, NVec4f(0.65f, .2f, .15f, 1.0f));
  //zep.SetColor(ThemeColor::FlashColor, NVec4f(.80f, .40f, .05f, 1.0f));
  //zep.SetColor(ThemeColor::HiddenText, NVec4f(.9f, .1f, .1f, 1.0f));
  //zep.SetColor(ThemeColor::Identifier, NVec4f(1.0f, .75f, 0.5f, 1.0f));
  //zep.SetColor(ThemeColor::Info, NVec4f(0.15f, .6f, .15f, 1.0f));
  zep.SetColor(ThemeColor::Keyword, NVec4f(.5216*1.1, .7333*1.1, .80*1.1, 1));
  //zep.SetColor(ThemeColor::Light, NVec4f(1.0f));
  zep.SetColor(ThemeColor::LineNumberActive, NVec4f(.4039*1.2, .4314*1.2, .4706*1.2, 1));
  zep.SetColor(ThemeColor::LineNumber, NVec4f(.4039, .4314, .4706, 1));
  zep.SetColor(ThemeColor::Mode, NVec4f(0.92f, 0.18f, 0.29f, 1.00f));
  zep.SetColor(ThemeColor::Number, NVec4f(.8196, .6039, .40, 1));
  zep.SetColor(ThemeColor::String, NVec4f(.5961, .7647, .4745, 1));
  zep.SetColor(ThemeColor::TabActive, zep.GetColor(ThemeColor::Background) * 1.55);
  zep.SetColor(ThemeColor::TabInactive, zep.GetColor(ThemeColor::Background) * 1.35);
  //zep.SetColor(ThemeColor::TabBorder, NVec4f(.55f, .55f, .55f, 1.0f));
  //zep.SetColor(ThemeColor::TextDim, NVec4f(.45f, .45f, .45f, 1.0f));
  zep.SetColor(ThemeColor::Text, NVec4f(0.87f, 0.94f, 0.9f, 1.0f));
  zep.SetColor(ThemeColor::VisualSelectBackground, NVec4f(0.20*1.3, 0.2157*1.3, 0.2627*1.3, 1));
  //zep.SetColor(ThemeColor::Warning, );
  zep.SetColor(ThemeColor::Whitespace, NVec4f(0.20*1.25, 0.2157*1.25, 0.2627*1.25, 1));
  zep.SetColor(ThemeColor::WidgetBackground, NVec4f(0.20, 0.2157, 0.2627, 1));
  zep.SetColor(ThemeColor::WidgetActive, NVec4f(0.92f, 0.18f, 0.29f, 1.00f));
  zep.SetColor(ThemeColor::WidgetInactive, zep.GetColor(ThemeColor::Background) + NVec4f(.02f, .02f, .02f, 0.0f));
  //zep.SetColor(ThemeColor::WidgetBorder, NVec4f(.5f, .5f, .5f, 1.0f));

  zep.SetColor(ThemeColor::LineNumberBackground, zep.GetColor(ThemeColor::Background) + NVec4f(.02f, .02f, .02f, 0.0f));
  zep.SetColor(ThemeColor::Normal, zep.GetColor(ThemeColor::Text));
  //zep.SetColor(ThemeColor::Parenthesis, zep.GetColor(ThemeColor::Text)/* NVec4f(.8549, .8667, .8941, 1) */);
  //zep.SetColor(ThemeColor::WidgetActive, zep.GetColor(ThemeColor::TabActive));
  //zep.SetColor(ThemeColor::WidgetInactive, zep.GetColor(ThemeColor::TabInactive));
  // clang-format on
}
