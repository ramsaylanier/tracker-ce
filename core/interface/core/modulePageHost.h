#ifndef INTERFACE_CORE_MODULEPAGEHOST_H_
#define INTERFACE_CORE_MODULEPAGEHOST_H_

#include "displayStructs.h"
#include "sdk/Ids.h"
#include "sdk/ModulePage.h"
#include "sdk/SoftKey.h"
#include "sdk/UiKit.h"

#include <stdint.h>

// Callbacks a ModuleBase screen provides so ModulePageHost can drive its chrome.
struct ScreenChrome
{
    hControl* softLabels = nullptr; // label[0..7]
    void (*setTitle)(const char* title) = nullptr;
    void (*setListCount)(uint8_t count) = nullptr;
    void (*setListRow)(uint8_t index, const char* name, const char* value) = nullptr;
    void (*setListFocus)(uint8_t index, bool editingValue) = nullptr;
    void (*refresh)() = nullptr;
    void (*onClose)() = nullptr;
    bool (*isShiftPressed)() = nullptr;
};

class ModulePageHost : public PageHost
{
  public:
    explicit constexpr ModulePageHost(ScreenId screen)
      : screenId(screen)
    {
    }

    void bind(const ScreenChrome& chrome);

    void setTitle(const char* title) override;
    void setSoftKey(uint8_t index,
                    const char* text,
                    LabelState state,
                    const char* text2 = nullptr) override;
    void finishSoftKeys() override;
    void setListCount(uint8_t count) override;
    void setListRow(uint8_t index, const char* name, const char* value) override;
    void setListFocus(uint8_t index, bool editingValue) override;
    void refresh() override;
    void close() override;
    bool isShiftPressed() const override;
    DeviceKind device() const override;

    // Soft-key helpers for builtin pages.
    void applySoftKeys(uint8_t builtinPageId);
    bool handleSoftKey(uint8_t builtinPageId, uint8_t slot, SoftKeyButtonState state);

  private:
    // Display controls keep the pointer they are given, so the host owns copies of every
    // string a module passes in. Modules may format into stack buffers.
    static constexpr uint8_t kTitleLen = 40;
    static constexpr uint8_t kSoftKeyTextLen = 14;

    const ScreenId screenId;
    ScreenChrome chrome{};
    char titleText[kTitleLen]{};
    char softKeyText[8][kSoftKeyTextLen]{};
    char softKeyText2[8][kSoftKeyTextLen]{};
};

// Per-screen view of the ExtensionCatalog: module page lookup, Mini menu modes and
// soft-key overrides. Each screen keeps track of its own active module page.
//
// A screen's pages form one list: its builtin pages first (menu mode index == BuiltinPage
// id), then the module pages added with addPage(). A builtin page can be stood in for by
// a module page registered with replacePage().
class ScreenExtensionPoint
{
  public:
    constexpr ScreenExtensionPoint(ScreenId screen, ModulePageHost& host)
      : screenId(screen)
      , host(host)
    {
    }

    // Pages added after the builtin pages.
    uint8_t modulePageCount() const;
    ModulePage* modulePageAt(uint8_t index) const;

    // Module page registered to replace a builtin page, or nullptr.
    ModulePage* replacementFor(uint8_t builtinPageId) const;
    // Position of a module page in the screen's page list: the builtin page it replaces,
    // or builtinModeCount + its index among the added pages. -1 if it is not on this screen.
    int8_t modeIndexForPage(const ModulePage* page, uint8_t builtinModeCount) const;

    // Mini menu dots: builtin modes + module pages. Returns 0 (no dots) for a
    // single-mode screen that has no module pages.
    uint8_t menuModeCount(uint8_t builtinModeCount) const;
    // option: 1..builtinModeCount = builtin; BuiltinPage::kModulePageIdBase+i = module page.
    uint32_t menuModeOption(uint8_t builtinModeCount, uint8_t index) const;
    ModulePage* pageForMenuOption(uint32_t option) const;
    uint8_t modeIndexForOption(uint8_t builtinModeCount, uint32_t option) const;

    void applySoftKeys(uint8_t builtinPageId);
    bool handleSoftKey(uint8_t builtinPageId, uint8_t slot, SoftKeyButtonState state);

  private:
    const ScreenId screenId;
    ModulePageHost& host;
};

// Shared host instances used by Song / Performance / Master / Instrument / Pattern.
ModulePageHost& getSongPageHost();
ModulePageHost& getPerformancePageHost();
ModulePageHost& getMasterPageHost();
ModulePageHost& getInstrumentPageHost();
ModulePageHost& getPatternPageHost();

ScreenExtensionPoint& songExtensions();
ScreenExtensionPoint& performanceExtensions();
ScreenExtensionPoint& masterExtensions();
ScreenExtensionPoint& instrumentExtensions();
ScreenExtensionPoint& patternExtensions();

// Main-thread playlist replace notify (FileManager / Song import).
void notifySongPlaylistReplaced();

// Main-thread playlist insert/remove (Song editor). Also notifies SongColumn.
void notifySongInserted(uint8_t index, bool duplicate);
void notifySongRemoved(uint8_t index);

// Main-thread song playback edge detect → SongColumn::onSlotStart / listeners.
void notifySongPlaybackTick();

#endif /* INTERFACE_CORE_MODULEPAGEHOST_H_ */
