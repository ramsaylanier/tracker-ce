#include "Registrar.h"

static ExtensionCatalog s_catalog;

namespace
{
// Builtin pages a module page can stand in for: ids 0..count-1 on each screen.
// Only Master currently resolves ScreenExtensionPoint::replacementFor().
uint8_t replaceablePageCount(ScreenId screen)
{
    switch (screen)
    {
        case ScreenId::Master: return 3;
        default: return 0;
    }
}

// True for a page that addPage() appended to `screen` (not a replacement).
bool isAddedPageOn(const PageRegistration& reg, ScreenId screen)
{
    return reg.screen == screen && reg.replacesBuiltin == PageRegistration::kAddedPage;
}

// Builtin pages whose soft keys the host screen lets modules take over.
bool acceptsSoftKeys(ScreenId screen, uint8_t builtinPageId)
{
    return (screen == ScreenId::Pattern && builtinPageId == BuiltinPage::Pattern::Main) ||
           (screen == ScreenId::Instrument && builtinPageId == BuiltinPage::Instrument::Effects);
}
} // namespace

ExtensionCatalog& extensionCatalog()
{
    return s_catalog;
}

ExtensionRegistrar::ExtensionRegistrar(ExtensionCatalog& catalog_)
  : catalog(catalog_)
{
}

bool ExtensionRegistrar::addPage(ScreenId screen, ModulePage* page, uint8_t order)
{
    PageRegistration reg;
    reg.screen = screen;
    reg.page = page;
    reg.order = order;
    return catalog.addPage(reg);
}

bool ExtensionRegistrar::replacePage(ScreenId screen, uint8_t builtinPageId, ModulePage* page)
{
    PageRegistration reg;
    reg.screen = screen;
    reg.page = page;
    reg.order = 0;
    reg.replacesBuiltin = builtinPageId;
    return catalog.addPage(reg);
}

bool ExtensionRegistrar::addSoftKey(ScreenId screen,
                                    uint8_t builtinPageId,
                                    uint8_t slot,
                                    SoftKeyHandler* handler)
{
    SoftKeyRegistration reg;
    reg.screen = screen;
    reg.builtinPageId = builtinPageId;
    reg.slot = slot;
    reg.handler = handler;
    return catalog.addSoftKey(reg);
}

bool ExtensionRegistrar::addSongColumn(SongColumn* column)
{
    return catalog.addSongColumn(column);
}

bool ExtensionRegistrar::addPatternColumn(PatternColumn* column)
{
    return catalog.addPatternColumn(column);
}

bool ExtensionRegistrar::addPatternStepFx(const PatternStepFxInfo* info,
                                          PatternStepFxHandler handler)
{
    PatternStepFxRegistration reg;
    reg.info = info;
    reg.handler = handler;
    return catalog.addPatternStepFx(reg);
}

bool ExtensionRegistrar::addConfigItem(ConfigItem* item)
{
    return catalog.addConfigItem(item);
}

bool ExtensionRegistrar::addEventListener(ModuleEventListener* listener)
{
    return catalog.addEventListener(listener);
}

void ExtensionCatalog::clear()
{
    pageCount_ = 0;
    softKeyCount_ = 0;
    songColumnCount_ = 0;
    patternColumnCount_ = 0;
    stepFxCount_ = 0;
    configItemCount_ = 0;
    listenerCount_ = 0;
}

bool ExtensionCatalog::addPage(const PageRegistration& reg)
{
    if (reg.page == nullptr || pageCount_ >= kMaxModulePages)
    {
        return false;
    }

    if (reg.replacesBuiltin != PageRegistration::kAddedPage)
    {
        if (reg.replacesBuiltin >= replaceablePageCount(reg.screen))
        {
            return false;
        }
        for (uint8_t i = 0; i < pageCount_; i++)
        {
            if (pages_[i].screen == reg.screen && pages_[i].replacesBuiltin == reg.replacesBuiltin)
            {
                return false;
            }
        }
    }

    // Insert sorted by (screen, order, name-ish pointer order).
    uint8_t insertAt = pageCount_;
    for (uint8_t i = 0; i < pageCount_; i++)
    {
        if (pages_[i].screen > reg.screen ||
            (pages_[i].screen == reg.screen && pages_[i].order > reg.order))
        {
            insertAt = i;
            break;
        }
    }

    for (uint8_t i = pageCount_; i > insertAt; i--)
    {
        pages_[i] = pages_[i - 1];
    }
    pages_[insertAt] = reg;
    pageCount_++;
    return true;
}

bool ExtensionCatalog::addSoftKey(const SoftKeyRegistration& reg)
{
    if (reg.handler == nullptr || reg.slot >= 8 || softKeyCount_ >= kMaxSoftKeyHandlers)
    {
        return false;
    }
    if (!acceptsSoftKeys(reg.screen, reg.builtinPageId))
    {
        return false;
    }

    for (uint8_t i = 0; i < softKeyCount_; i++)
    {
        if (softKeys_[i].screen == reg.screen && softKeys_[i].builtinPageId == reg.builtinPageId &&
            softKeys_[i].slot == reg.slot)
        {
            return false;
        }
    }

    softKeys_[softKeyCount_++] = reg;
    return true;
}

bool ExtensionCatalog::addSongColumn(SongColumn* column)
{
    if (column == nullptr || songColumnCount_ >= kMaxSongColumns)
    {
        return false;
    }
    songColumns_[songColumnCount_++] = column;
    return true;
}

bool ExtensionCatalog::addPatternColumn(PatternColumn* column)
{
    if (column == nullptr || patternColumnCount_ >= kMaxPatternColumns)
    {
        return false;
    }
    patternColumns_[patternColumnCount_++] = column;
    return true;
}

bool ExtensionCatalog::isPatternStepFxLetterTaken(char letter) const
{
    for (uint8_t i = 0; i < stepFxCount_; i++)
    {
        if (stepFx_[i].info != nullptr && stepFx_[i].info->letter == letter)
        {
            return true;
        }
    }
    return false;
}

bool ExtensionCatalog::addPatternStepFx(const PatternStepFxRegistration& reg)
{
    if (reg.info == nullptr || stepFxCount_ >= kMaxModulePatternStepFx)
    {
        return false;
    }
    if (reg.info->typeId < kPatternStepFxModuleIdBase ||
        reg.info->typeId > kPatternStepFxModuleIdLast)
    {
        return false;
    }
    if (reg.info->letter == '\0' || reg.info->name == nullptr || reg.info->name[0] == '\0')
    {
        return false;
    }
    if (findPatternStepFx(reg.info->typeId) != nullptr)
    {
        return false;
    }
    if (isPatternStepFxLetterTaken(reg.info->letter))
    {
        return false;
    }

    stepFx_[stepFxCount_++] = reg;
    return true;
}

bool ExtensionCatalog::addConfigItem(ConfigItem* item)
{
    if (item == nullptr || configItemCount_ >= kMaxConfigItems)
    {
        return false;
    }
    configItems_[configItemCount_++] = item;
    return true;
}

bool ExtensionCatalog::addEventListener(ModuleEventListener* listener)
{
    if (listener == nullptr || listenerCount_ >= kMaxEventListeners)
    {
        return false;
    }
    listeners_[listenerCount_++] = listener;
    return true;
}

uint8_t ExtensionCatalog::pageCount(ScreenId screen) const
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < pageCount_; i++)
    {
        if (isAddedPageOn(pages_[i], screen))
        {
            n++;
        }
    }
    return n;
}

ModulePage* ExtensionCatalog::pageAt(ScreenId screen, uint8_t index) const
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < pageCount_; i++)
    {
        if (!isAddedPageOn(pages_[i], screen))
        {
            continue;
        }
        if (n == index)
        {
            return pages_[i].page;
        }
        n++;
    }
    return nullptr;
}

ModulePage* ExtensionCatalog::findPageReplacement(ScreenId screen, uint8_t builtinPageId) const
{
    for (uint8_t i = 0; i < pageCount_; i++)
    {
        if (pages_[i].screen == screen && pages_[i].replacesBuiltin == builtinPageId)
        {
            return pages_[i].page;
        }
    }
    return nullptr;
}

SoftKeyHandler* ExtensionCatalog::softKeyAt(ScreenId screen,
                                            uint8_t builtinPageId,
                                            uint8_t slot) const
{
    for (uint8_t i = 0; i < softKeyCount_; i++)
    {
        if (softKeys_[i].screen == screen && softKeys_[i].builtinPageId == builtinPageId &&
            softKeys_[i].slot == slot)
        {
            return softKeys_[i].handler;
        }
    }
    return nullptr;
}

SongColumn* ExtensionCatalog::firstSongColumn() const
{
    return songColumnCount_ > 0 ? songColumns_[0] : nullptr;
}

PatternColumn* ExtensionCatalog::patternColumnAt(uint8_t index) const
{
    if (index >= patternColumnCount_)
    {
        return nullptr;
    }
    return patternColumns_[index];
}

const PatternStepFxInfo* ExtensionCatalog::patternStepFxAt(uint8_t index) const
{
    if (index >= stepFxCount_)
    {
        return nullptr;
    }
    return stepFx_[index].info;
}

const PatternStepFxInfo* ExtensionCatalog::findPatternStepFx(uint8_t typeId) const
{
    for (uint8_t i = 0; i < stepFxCount_; i++)
    {
        if (stepFx_[i].info != nullptr && stepFx_[i].info->typeId == typeId)
        {
            return stepFx_[i].info;
        }
    }
    return nullptr;
}

bool ExtensionCatalog::dispatchPatternStepFx(uint8_t typeId, uint8_t value)
{
    for (uint8_t i = 0; i < stepFxCount_; i++)
    {
        if (stepFx_[i].info != nullptr && stepFx_[i].info->typeId == typeId)
        {
            if (stepFx_[i].handler != nullptr)
            {
                return stepFx_[i].handler(typeId, value);
            }
            return false;
        }
    }
    return false;
}

ConfigItem* ExtensionCatalog::configItemAt(uint8_t index) const
{
    if (index >= configItemCount_)
    {
        return nullptr;
    }
    return configItems_[index];
}

void ExtensionCatalog::notifySongSlotStart(uint8_t slot)
{
    for (uint8_t i = 0; i < listenerCount_; i++)
    {
        listeners_[i]->onSongSlotStart(slot);
    }
    if (SongColumn* col = firstSongColumn())
    {
        col->onSlotStart(slot);
    }
}

void ExtensionCatalog::notifySongPlaylistReplaced()
{
    for (uint8_t i = 0; i < listenerCount_; i++)
    {
        listeners_[i]->onSongPlaylistReplaced();
    }
    if (SongColumn* col = firstSongColumn())
    {
        col->clearAll();
    }
}

void ExtensionCatalog::notifySongInserted(uint8_t index, bool duplicate)
{
    for (uint8_t i = 0; i < listenerCount_; i++)
    {
        listeners_[i]->onSongInserted(index, duplicate);
    }
    if (SongColumn* col = firstSongColumn())
    {
        col->onInserted(index, duplicate);
    }
}

void ExtensionCatalog::notifySongRemoved(uint8_t index)
{
    for (uint8_t i = 0; i < listenerCount_; i++)
    {
        listeners_[i]->onSongRemoved(index);
    }
    if (SongColumn* col = firstSongColumn())
    {
        col->onRemoved(index);
    }
}

void ExtensionCatalog::notifyPatternLoad(uint16_t patternIdx)
{
    for (uint8_t i = 0; i < listenerCount_; i++)
    {
        listeners_[i]->onPatternLoad(patternIdx);
    }
    for (uint8_t c = 0; c < patternColumnCount_; c++)
    {
        patternColumns_[c]->onPatternLoad(patternIdx);
    }
}

void ExtensionCatalog::notifyPatternSave(uint16_t patternIdx)
{
    for (uint8_t i = 0; i < listenerCount_; i++)
    {
        listeners_[i]->onPatternSave(patternIdx);
    }
    for (uint8_t c = 0; c < patternColumnCount_; c++)
    {
        patternColumns_[c]->onPatternSave(patternIdx);
    }
}
