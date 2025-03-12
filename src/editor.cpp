// Sourced from:
// https://github.com/cmaughan/zep_imgui/blob/main/demo/src/editor.cpp

#if ZEP_SINGLE_HEADER == 1
#define ZEP_SINGLE_HEADER_BUILD
#endif

#include "editor.h"
#include "app_path.h"
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

using namespace Zep;

using cmdFunc = std::function<void(const std::vector<std::string> &)>;
class ZepCmd : public ZepExCommand {
public:
  ZepCmd(ZepEditor &editor, const std::string name, cmdFunc fn)
      : ZepExCommand(editor), m_name(name), m_func(fn) {}

  virtual void Run(const std::vector<std::string> &args) override { m_func(args); }

  virtual const char *ExCommandName() const override { return m_name.c_str(); }

private:
  std::string m_name;
  cmdFunc m_func;
};

struct ZepWrapper : public Zep::IZepComponent {
  ZepWrapper(const fs::path &root_path, const Zep::NVec2f &pixelScale,
             std::function<void(std::shared_ptr<Zep::ZepMessage>)> fnCommandCB)
      : zepEditor(fs::path(root_path.string()), pixelScale), Callback(fnCommandCB) {
    zepEditor.RegisterCallback(this);
  }

  virtual Zep::ZepEditor &GetEditor() const override { return (Zep::ZepEditor &)zepEditor; }

  virtual void Notify(std::shared_ptr<Zep::ZepMessage> message) override {
    Callback(message);

    return;
  }

  virtual void HandleInput() { zepEditor.HandleInput(); }

  Zep::ZepEditor_ImGui zepEditor;
  std::function<void(std::shared_ptr<Zep::ZepMessage>)> Callback;
};

std::shared_ptr<ZepWrapper> spZep;

void zep_init(const Zep::NVec2f &pixelScale,
              std::function<void(std::shared_ptr<Zep::ZepMessage>)> callback) {
  // Initialize the editor and watch for changes
  spZep =
      std::make_shared<ZepWrapper>(getAppDir(), Zep::NVec2f(pixelScale.x, pixelScale.y), callback);

  // This is an example of adding different fonts for text styles.
  // If you ":e test.md" in the editor and type "# Heading 1" you will
  // see that Zep picks a different font size for the heading.
  auto &display = spZep->GetEditor().GetDisplay();
  auto pImFont = ImGui::GetIO().Fonts[0].Fonts[0];
  auto pixelHeight = pImFont->FontSize;
  display.SetFont(ZepTextType::UI,
                  std::make_shared<ZepFont_ImGui>(display, pImFont, int(pixelHeight)));
  display.SetFont(ZepTextType::Text,
                  std::make_shared<ZepFont_ImGui>(display, pImFont, int(pixelHeight)));
  display.SetFont(ZepTextType::Heading1,
                  std::make_shared<ZepFont_ImGui>(display, pImFont, int(pixelHeight * 1.5)));
  display.SetFont(ZepTextType::Heading2,
                  std::make_shared<ZepFont_ImGui>(display, pImFont, int(pixelHeight * 1.25)));
  display.SetFont(ZepTextType::Heading3,
                  std::make_shared<ZepFont_ImGui>(display, pImFont, int(pixelHeight * 1.125)));
}

void zep_update() {
  // This is required to make the editor cursor blink, and for the :ZTestFlash
  // example
  if (spZep) {
    spZep->GetEditor().RefreshRequired();
  }
}

void zep_destroy() { spZep.reset(); }

ZepEditor &zep_get_editor() { return spZep->GetEditor(); }

void zep_show() {
  auto min = ImGui::GetCursorScreenPos();
  auto max = ImGui::GetContentRegionAvail();
  if (max.x <= 0)
    max.x = 1;
  if (max.y <= 0)
    max.y = 1;
  ImGui::InvisibleButton("ZepContainer", max);

  // Fill the window
  max.x = min.x + max.x;
  max.y = min.y + max.y;

  spZep->zepEditor.SetDisplayRegion(Zep::NVec2f(min.x, min.y), Zep::NVec2f(max.x, max.y));
  spZep->zepEditor.Display();
  bool zep_focused = ImGui::IsWindowFocused();
  if (zep_focused) {
    spZep->zepEditor.HandleInput();
  }
}
