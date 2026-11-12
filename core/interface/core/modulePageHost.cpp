#include "modulePageHost.h"

#include "core/interfaceDefs.h"
#include "display.h"
#include "mtSequencer.h"
#include "mtStructs.h"
#include "sdk/Registrar.h"

#include <cstdio>

static ModulePageHost s_songPageHost{ ScreenId::Song };
static ModulePageHost s_performancePageHost{ ScreenId::Performance };
static ModulePageHost s_masterPageHost{ ScreenId::Master };
static ModulePageHost s_instrumentPageHost{ ScreenId::Instrument };
static ModulePageHost s_patternPageHost{ ScreenId::Pattern };

static ScreenExtensionPoint s_songExt{ ScreenId::Song, s_songPageHost };
static ScreenExtensionPoint s_performanceExt{ ScreenId::Performance, s_performancePageHost };
static ScreenExtensionPoint s_masterExt{ ScreenId::Master, s_masterPageHost };
static ScreenExtensionPoint s_instrumentExt{ ScreenId::Instrument, s_instrumentPageHost };
static ScreenExtensionPoint s_patternExt{ ScreenId::Pattern, s_patternPageHost };

ModulePageHost& getSongPageHost()
{
    return s_songPageHost;
}
ModulePageHost& getPerformancePageHost()
{
    return s_performancePageHost;
}
ModulePageHost& getMasterPageHost()
{
    return s_masterPageHost;
}
ModulePageHost& getInstrumentPageHost()
{
    return s_instrumentPageHost;
}
ModulePageHost& getPatternPageHost()
{
    return s_patternPageHost;
}

ScreenExtensionPoint& songExtensions()
{
    return s_songExt;
}
ScreenExtensionPoint& performanceExtensions()
{
    return s_performanceExt;
}
ScreenExtensionPoint& masterExtensions()
{
    return s_masterExt;
}
ScreenExtensionPoint& instrumentExtensions()
{
    return s_instrumentExt;
}
ScreenExtensionPoint& patternExtensions()
{
    return s_patternExt;
}

void ModulePageHost::bind(const ScreenChrome& chrome_)
{
    chrome = chrome_;
}

void ModulePageHost::setTitle(const char* title)
{
    if (chrome.setTitle != nullptr)
    {
        snprintf(titleText, sizeof(titleText), "%s", title != nullptr ? title : "");
        chrome.setTitle(titleText);
    }
}

void ModulePageHost::setSoftKey(uint8_t index,
                                const char* text,
                                LabelState state,
                                const char* text2)
{
    if (chrome.softLabels == nullptr || index >= 8)
    {
        return;
    }

    const uint32_t* colors = display.getTheme().inactiveLabelsColors;
    if (state == LabelState::Active || state == LabelState::Filled)
    {
        colors = display.getTheme().performActiveTrackLabelsColors;
    }
    else if (state == LabelState::Muted)
    {
        colors = display.getTheme().performMutedTrackLabelsColors;
    }

    const char* secondary = text2;
    if (secondary == nullptr && state == LabelState::Active)
    {
        secondary = "Active";
    }

    snprintf(softKeyText[index], kSoftKeyTextLen, "%s", text != nullptr ? text : "");
    snprintf(softKeyText2[index], kSoftKeyTextLen, "%s", secondary != nullptr ? secondary : "");

    display.setControlColors(chrome.softLabels[index], colors);
    display.setControlText(chrome.softLabels[index], softKeyText[index]);
    display.setControlText2(chrome.softLabels[index], softKeyText2[index]);
    display.setControlStyle(chrome.softLabels[index],
                            controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
    display.setControlShow(chrome.softLabels[index]);
    display.refreshControl(chrome.softLabels[index]);
}

void ModulePageHost::finishSoftKeys()
{
    display.synchronizeRefresh();
}

void ModulePageHost::setListCount(uint8_t count)
{
    if (chrome.setListCount != nullptr)
    {
        chrome.setListCount(count);
    }
}

void ModulePageHost::setListRow(uint8_t index, const char* name, const char* value)
{
    if (chrome.setListRow != nullptr)
    {
        chrome.setListRow(index, name, value);
    }
}

void ModulePageHost::setListFocus(uint8_t index, bool editingValue)
{
    if (chrome.setListFocus != nullptr)
    {
        chrome.setListFocus(index, editingValue);
    }
}

void ModulePageHost::refresh()
{
    if (chrome.refresh != nullptr)
    {
        chrome.refresh();
    }
}

void ModulePageHost::close()
{
    if (chrome.onClose != nullptr)
    {
        chrome.onClose();
    }
}

bool ModulePageHost::isShiftPressed() const
{
    if (chrome.isShiftPressed != nullptr)
    {
        return chrome.isShiftPressed();
    }
    return false;
}

DeviceKind ModulePageHost::device() const
{
#ifdef TRACKER_MINI
    return DeviceKind::Mini;
#elif defined(PLATFORM_IMX)
    return DeviceKind::Plus;
#else
    return DeviceKind::Original;
#endif
}

void ModulePageHost::applySoftKeys(uint8_t builtinPageId)
{
    for (uint8_t slot = 0; slot < 8; slot++)
    {
        SoftKeyHandler* handler =
          extensionCatalog().softKeyAt(screenId, builtinPageId, slot);
        if (handler == nullptr)
        {
            continue;
        }
        setSoftKey(slot, handler->label(), handler->state(), handler->label2());
    }
}

bool ModulePageHost::handleSoftKey(uint8_t builtinPageId, uint8_t slot, SoftKeyButtonState state)
{
    SoftKeyHandler* handler = extensionCatalog().softKeyAt(screenId, builtinPageId, slot);
    if (handler == nullptr)
    {
        return false;
    }
    return handler->onPress(*this, state, isShiftPressed());
}

uint8_t ScreenExtensionPoint::modulePageCount() const
{
    return extensionCatalog().pageCount(screenId);
}

ModulePage* ScreenExtensionPoint::modulePageAt(uint8_t index) const
{
    return extensionCatalog().pageAt(screenId, index);
}

ModulePage* ScreenExtensionPoint::replacementFor(uint8_t builtinPageId) const
{
    return extensionCatalog().findPageReplacement(screenId, builtinPageId);
}

int8_t ScreenExtensionPoint::modeIndexForPage(const ModulePage* page,
                                              uint8_t builtinModeCount) const
{
    if (page == nullptr)
    {
        return -1;
    }
    for (uint8_t id = 0; id < builtinModeCount; id++)
    {
        if (replacementFor(id) == page)
        {
            return static_cast<int8_t>(id);
        }
    }
    const uint8_t added = modulePageCount();
    for (uint8_t i = 0; i < added; i++)
    {
        if (modulePageAt(i) == page)
        {
            return static_cast<int8_t>(builtinModeCount + i);
        }
    }
    return -1;
}

uint8_t ScreenExtensionPoint::menuModeCount(uint8_t builtinModeCount) const
{
    const uint8_t extra = modulePageCount();
    if (extra == 0 && builtinModeCount <= 1)
    {
        return 0;
    }
    return static_cast<uint8_t>(builtinModeCount + extra);
}

uint32_t ScreenExtensionPoint::menuModeOption(uint8_t builtinModeCount, uint8_t index) const
{
    if (index < builtinModeCount)
    {
        return index + 1; // builtin options are 1-based
    }
    const uint8_t moduleIndex = static_cast<uint8_t>(index - builtinModeCount);
    return BuiltinPage::kModulePageIdBase + moduleIndex;
}

ModulePage* ScreenExtensionPoint::pageForMenuOption(uint32_t option) const
{
    if (option < BuiltinPage::kModulePageIdBase)
    {
        return nullptr;
    }
    const uint8_t moduleIndex =
      static_cast<uint8_t>(option - BuiltinPage::kModulePageIdBase);
    return modulePageAt(moduleIndex);
}

uint8_t ScreenExtensionPoint::modeIndexForOption(uint8_t builtinModeCount, uint32_t option) const
{
    if (option < BuiltinPage::kModulePageIdBase)
    {
        if (option == 0)
        {
            return 0;
        }
        return static_cast<uint8_t>(option - 1);
    }
    const uint8_t moduleIndex =
      static_cast<uint8_t>(option - BuiltinPage::kModulePageIdBase);
    return static_cast<uint8_t>(builtinModeCount + moduleIndex);
}

void ScreenExtensionPoint::applySoftKeys(uint8_t builtinPageId)
{
    host.applySoftKeys(builtinPageId);
}

bool ScreenExtensionPoint::handleSoftKey(uint8_t builtinPageId,
                                         uint8_t slot,
                                         SoftKeyButtonState state)
{
    return host.handleSoftKey(builtinPageId, slot, state);
}

void notifySongPlaylistReplaced()
{
    extensionCatalog().notifySongPlaylistReplaced();
}

void notifySongPlaybackTick()
{
    static bool wasPlayingSong = false;
    static int16_t lastRecalledSongSlot = -1;

    const bool playingSong = sequencer.isPlay() && sequencer.isSongMode();
    if (playingSong)
    {
        const uint8_t songSlot = mtProject.song.playlistPos;
        if (!wasPlayingSong || lastRecalledSongSlot != static_cast<int16_t>(songSlot))
        {
            lastRecalledSongSlot = songSlot;
            extensionCatalog().notifySongSlotStart(songSlot);
        }
        wasPlayingSong = true;
        return;
    }

    wasPlayingSong = false;
    lastRecalledSongSlot = -1;
}
