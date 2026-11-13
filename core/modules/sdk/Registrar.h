#ifndef MODULES_SDK_REGISTRAR_H_
#define MODULES_SDK_REGISTRAR_H_

#include "Columns.h"
#include "ConfigItem.h"
#include "Events.h"
#include "Ids.h"
#include "ModulePage.h"
#include "PatternStepFx.h"
#include "SoftKey.h"

#include <stdint.h>

using PatternStepFxHandler = bool (*)(uint8_t typeId, uint8_t value);

// Capacity limits for the static ExtensionCatalog.
static constexpr uint8_t kMaxModulePages = 16;
static constexpr uint8_t kMaxSoftKeyHandlers = 16;
// Hosts show only the first registered column (first-wins).
static constexpr uint8_t kMaxSongColumns = 1;
static constexpr uint8_t kMaxPatternColumns = 1;
static constexpr uint8_t kMaxConfigItems = 16;
static constexpr uint8_t kMaxEventListeners = 8;

struct PageRegistration
{
    // replacesBuiltin value of a page that is added after the builtin pages.
    static constexpr uint8_t kAddedPage = 0xFF;

    ScreenId screen = ScreenId::Song;
    ModulePage* page = nullptr;
    uint8_t order = 100;
    uint8_t replacesBuiltin = kAddedPage; // else the BuiltinPage id it stands in for
};

struct SoftKeyRegistration
{
    ScreenId screen = ScreenId::Instrument;
    uint8_t builtinPageId = 0;
    uint8_t slot = 0;
    SoftKeyHandler* handler = nullptr;
};

struct PatternStepFxRegistration
{
    const PatternStepFxInfo* info = nullptr;
    PatternStepFxHandler handler = nullptr;
};

class ExtensionCatalog;

// Modules call this from FeatureModule::registerExtensions.
class ExtensionRegistrar
{
  public:
    explicit ExtensionRegistrar(ExtensionCatalog& catalog);

    // Every method returns false when the registration is rejected: a full table, a
    // conflict with an earlier registration, or a target the host screen does not support.

    // Appends a page after the screen's builtin pages; lower order comes first.
    bool addPage(ScreenId screen, ModulePage* page, uint8_t order = 100);
    // Shows `page` wherever the screen would show the builtin page. See BuiltinPage in
    // Ids.h for the pages that can be replaced; one replacement per builtin page.
    bool replacePage(ScreenId screen, uint8_t builtinPageId, ModulePage* page);
    // Takes over soft key `slot` (0..7) of a builtin page. See BuiltinPage in Ids.h for
    // the pages that accept soft keys.
    bool addSoftKey(ScreenId screen, uint8_t builtinPageId, uint8_t slot, SoftKeyHandler* handler);
    bool addSongColumn(SongColumn* column);
    bool addPatternColumn(PatternColumn* column);
    bool addPatternStepFx(const PatternStepFxInfo* info, PatternStepFxHandler handler);
    bool addConfigItem(ConfigItem* item);
    bool addEventListener(ModuleEventListener* listener);

  private:
    ExtensionCatalog& catalog;
};

class ExtensionCatalog
{
  public:
    void clear();

    // Registration (called during init via ExtensionRegistrar).
    bool addPage(const PageRegistration& reg);
    bool addSoftKey(const SoftKeyRegistration& reg);
    bool addSongColumn(SongColumn* column);
    bool addPatternColumn(PatternColumn* column);
    bool addPatternStepFx(const PatternStepFxRegistration& reg);
    bool addConfigItem(ConfigItem* item);
    bool addEventListener(ModuleEventListener* listener);

    // Queries
    uint8_t pageCount(ScreenId screen) const;
    ModulePage* pageAt(ScreenId screen, uint8_t index) const;
    ModulePage* findPageReplacement(ScreenId screen, uint8_t builtinPageId) const;

    SoftKeyHandler* softKeyAt(ScreenId screen, uint8_t builtinPageId, uint8_t slot) const;

    SongColumn* firstSongColumn() const;

    uint8_t patternColumnCount() const { return patternColumnCount_; }
    PatternColumn* patternColumnAt(uint8_t index) const;

    uint8_t patternStepFxCount() const { return stepFxCount_; }
    const PatternStepFxInfo* patternStepFxAt(uint8_t index) const;
    const PatternStepFxInfo* findPatternStepFx(uint8_t typeId) const;
    bool dispatchPatternStepFx(uint8_t typeId, uint8_t value);

    uint8_t configItemCount() const { return configItemCount_; }
    ConfigItem* configItemAt(uint8_t index) const;

    void notifySongSlotStart(uint8_t slot);
    void notifySongPlaylistReplaced();
    void notifySongInserted(uint8_t index, bool duplicate);
    void notifySongRemoved(uint8_t index);
    void notifyPatternLoad(uint16_t patternIdx);
    void notifyPatternSave(uint16_t patternIdx);

  private:
    PageRegistration pages_[kMaxModulePages] = {};
    uint8_t pageCount_ = 0;

    SoftKeyRegistration softKeys_[kMaxSoftKeyHandlers] = {};
    uint8_t softKeyCount_ = 0;

    SongColumn* songColumns_[kMaxSongColumns] = {};
    uint8_t songColumnCount_ = 0;

    PatternColumn* patternColumns_[kMaxPatternColumns] = {};
    uint8_t patternColumnCount_ = 0;

    PatternStepFxRegistration stepFx_[kMaxModulePatternStepFx] = {};
    uint8_t stepFxCount_ = 0;

    ConfigItem* configItems_[kMaxConfigItems] = {};
    uint8_t configItemCount_ = 0;

    ModuleEventListener* listeners_[kMaxEventListeners] = {};
    uint8_t listenerCount_ = 0;

    bool isPatternStepFxLetterTaken(char letter) const;
};

ExtensionCatalog& extensionCatalog();

#endif /* MODULES_SDK_REGISTRAR_H_ */
