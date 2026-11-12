#include "FxBuses.h"

#include "ModuleSidecar.h"
#include "Platform.h"
#include "SampleInstrumentFxQuery.h"

#include <cr_section_macros.h>
#include <cstdio>
#include <cstring>

__NOINIT(EXTERNAL_RAM) static strFxBusesFile externalRamFxBusesBank;

FxBuses fxBuses;

namespace
{
constexpr uint32_t kFxDirtyMask = 0xFFFFFFFFu;

static uint8_t clampSend(int sum)
{
    if (sum < 0)
        return 0;
    if (sum > (int)kFxBusSendMax)
        return kFxBusSendMax;
    return (uint8_t)sum;
}

static const strFxBus* providerBusForInstrument(uint8_t sampleInstrIdx)
{
    return fxBuses.busForInstrument(sampleInstrIdx);
}

static uint8_t providerFilterEnable(uint8_t sampleInstrIdx)
{
    if (const strFxBus* b = providerBusForInstrument(sampleInstrIdx))
        return b->filterEnable;
    return getEngineService().filterEnable(sampleInstrIdx);
}

static uint8_t providerFilterType(uint8_t sampleInstrIdx)
{
    if (const strFxBus* b = providerBusForInstrument(sampleInstrIdx))
        return b->filterType;
    return getEngineService().filterType(sampleInstrIdx);
}

static float providerCutOff(uint8_t sampleInstrIdx)
{
    if (const strFxBus* b = providerBusForInstrument(sampleInstrIdx))
        return b->cutOff;
    return getEngineService().cutOff(sampleInstrIdx);
}

static float providerResonance(uint8_t sampleInstrIdx)
{
    if (const strFxBus* b = providerBusForInstrument(sampleInstrIdx))
        return b->resonance;
    return getEngineService().resonance(sampleInstrIdx);
}

static uint8_t providerOverdrive(uint8_t sampleInstrIdx)
{
    if (const strFxBus* b = providerBusForInstrument(sampleInstrIdx))
        return b->overdrive;
    return getEngineService().overdrive(sampleInstrIdx);
}

static uint8_t providerBitDepth(uint8_t sampleInstrIdx)
{
    if (const strFxBus* b = providerBusForInstrument(sampleInstrIdx))
        return b->bitDepth;
    return getEngineService().bitDepth(sampleInstrIdx);
}

static uint8_t providerEffectiveDelaySend(uint8_t sampleInstrIdx)
{
    uint8_t base = getEngineService().delaySend(sampleInstrIdx);
    if (const strFxBus* b = providerBusForInstrument(sampleInstrIdx))
        return clampSend((int)base + (int)b->delaySend);
    return base;
}

static uint8_t providerEffectiveReverbSend(uint8_t sampleInstrIdx)
{
    uint8_t base = getEngineService().reverbSend(sampleInstrIdx);
    if (const strFxBus* b = providerBusForInstrument(sampleInstrIdx))
        return clampSend((int)base + (int)b->reverbSend);
    return base;
}

static const SampleInstrumentFxProvider kFxBusesProvider = {
    providerFilterEnable,
    providerFilterType,
    providerCutOff,
    providerResonance,
    providerOverdrive,
    providerBitDepth,
    providerEffectiveDelaySend,
    providerEffectiveReverbSend,
};

static const char* kParamNames[] = {
    "Filter", "Cutoff", "Resonance", "Drive", "Bit", "Delay", "Reverb",
};
static constexpr uint8_t kParamCount = 7;
} // namespace

FxBuses::FxBuses()
  : editBusSoftKey(*this)
  , masterPage(*this)
  , instrumentPage(*this)
{
}

void FxBuses::registerExtensions(ExtensionRegistrar& registrar)
{
    registrar.addPage(ScreenId::Master, &masterPage, 40);
    registrar.addPage(ScreenId::Instrument, &instrumentPage, 40);
    // Instrument Effects softkeys: slot 5 = Bus membership, slot 6 = Edit Bus
    registrar.addSoftKey(ScreenId::Instrument, BuiltinPage::Instrument::Effects, 5, this);
    registrar.addSoftKey(ScreenId::Instrument, BuiltinPage::Instrument::Effects, 6, &editBusSoftKey);
}

void FxBuses::initDefaultBus(strFxBus& bus, uint8_t index0Based)
{
    memset(&bus, 0, sizeof(bus));
    snprintf(bus.name, sizeof(bus.name), "Bus %u", (unsigned)(index0Based + 1));
    bus.cutOff = 1.0f;
    bus.resonance = 0.0f;
    bus.filterType = 0;
    bus.filterEnable = 0;
    bus.overdrive = 0;
    bus.bitDepth = kFxBusBitDepthMax;
    bus.delaySend = 0;
    bus.reverbSend = 0;
}

void FxBuses::clearBank()
{
    memset(&externalRamFxBusesBank, 0, sizeof(externalRamFxBusesBank));
    externalRamFxBusesBank.magic[0] = 'F';
    externalRamFxBusesBank.magic[1] = 'X';
    externalRamFxBusesBank.magic[2] = 'B';
    externalRamFxBusesBank.magic[3] = 'S';
    externalRamFxBusesBank.version = kFxBusesFileVersion;
    externalRamFxBusesBank.busCount = kFxBusCount;
    for (uint8_t i = 0; i < kFxBusCount; i++)
        initDefaultBus(externalRamFxBusesBank.buses[i], i);
}

void FxBuses::init(FileSystem* fileSystem_)
{
    this->fileSystem = fileSystem_;
    clearBank();
    setSampleInstrumentFxProvider(&kFxBusesProvider);
}

void FxBuses::onWorkspaceClear()
{
    clearBank();
}

void FxBuses::onWorkspaceLoad()
{
    char path[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::workspacePath(path, sizeof(path), id());
    if (!readFile(path))
    {
        clearBank();
        if (fileSystem != nullptr && fileSystem->exists(path))
            fileSystem->remove(path);
    }
}

void FxBuses::onProjectOpen(const char* projectPath, const char* projectName)
{
    char srcPath[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::projectPath(srcPath, sizeof(srcPath), projectPath, projectName, id());
    char workspacePathBuf[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::workspacePath(workspacePathBuf, sizeof(workspacePathBuf), id());
    if (!readFile(srcPath))
    {
        clearBank();
    }
    writeFile(workspacePathBuf);
}

void FxBuses::onProjectSave(const char* projectPath, const char* projectName)
{
    char destPath[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::projectPath(destPath, sizeof(destPath), projectPath, projectName, id());
    writeFile(destPath);
}

bool FxBuses::validateHeader() const
{
    return externalRamFxBusesBank.magic[0] == 'F' && externalRamFxBusesBank.magic[1] == 'X' &&
           externalRamFxBusesBank.magic[2] == 'B' && externalRamFxBusesBank.magic[3] == 'S' &&
           externalRamFxBusesBank.version == kFxBusesFileVersion &&
           externalRamFxBusesBank.busCount == kFxBusCount;
}

bool FxBuses::readFile(const char* path)
{
    if (!ModuleSidecar::read(
          fileSystem, path, &externalRamFxBusesBank, sizeof(externalRamFxBusesBank)))
        return false;
    if (!validateHeader())
    {
        clearBank();
        return false;
    }
    for (uint8_t i = 0; i < EngineService::kSampleInstrumentCount; i++)
    {
        if (externalRamFxBusesBank.instrumentFxBus[i] > kFxBusCount)
            externalRamFxBusesBank.instrumentFxBus[i] = kFxBusNone;
    }
    return true;
}

void FxBuses::writeFile(const char* path)
{
    externalRamFxBusesBank.magic[0] = 'F';
    externalRamFxBusesBank.magic[1] = 'X';
    externalRamFxBusesBank.magic[2] = 'B';
    externalRamFxBusesBank.magic[3] = 'S';
    externalRamFxBusesBank.version = kFxBusesFileVersion;
    externalRamFxBusesBank.busCount = kFxBusCount;
    ModuleSidecar::write(fileSystem, path, &externalRamFxBusesBank, sizeof(externalRamFxBusesBank));
}

void FxBuses::saveToWorkspace()
{
    char path[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::workspacePath(path, sizeof(path), id());
    writeFile(path);
}

uint8_t FxBuses::membership(uint8_t sampleInstrIdx) const
{
    if (sampleInstrIdx >= EngineService::kSampleInstrumentCount)
        return kFxBusNone;
    return externalRamFxBusesBank.instrumentFxBus[sampleInstrIdx];
}

void FxBuses::setMembership(uint8_t sampleInstrIdx, uint8_t busId)
{
    if (sampleInstrIdx >= EngineService::kSampleInstrumentCount)
        return;
    if (busId > kFxBusCount)
        busId = kFxBusNone;
    externalRamFxBusesBank.instrumentFxBus[sampleInstrIdx] = busId;
    saveToWorkspace();
    getEngineService().notifyInstrumentFxDirty(sampleInstrIdx, kFxDirtyMask);
}

strFxBus* FxBuses::bus(uint8_t busId1Based)
{
    if (busId1Based < 1 || busId1Based > kFxBusCount)
        return nullptr;
    return &externalRamFxBusesBank.buses[busId1Based - 1];
}

const strFxBus* FxBuses::bus(uint8_t busId1Based) const
{
    if (busId1Based < 1 || busId1Based > kFxBusCount)
        return nullptr;
    return &externalRamFxBusesBank.buses[busId1Based - 1];
}

strFxBus* FxBuses::busForInstrument(uint8_t sampleInstrIdx)
{
    return bus(membership(sampleInstrIdx));
}

const strFxBus* FxBuses::busForInstrument(uint8_t sampleInstrIdx) const
{
    return bus(membership(sampleInstrIdx));
}

void FxBuses::refreshMemberVoices(uint8_t busId1Based)
{
    if (busId1Based < 1 || busId1Based > kFxBusCount)
        return;
    for (uint8_t instr = 0; instr < EngineService::kSampleInstrumentCount; instr++)
    {
        if (membership(instr) != busId1Based)
            continue;
        getEngineService().notifyInstrumentFxDirty(instr, kFxDirtyMask);
    }
}

const char* FxBuses::label() const
{
    const uint8_t instr = getEngineService().selectedSampleInstrument();
    if (instr == 0xFF)
    {
        snprintf(membershipLabel, sizeof(membershipLabel), "Bus");
        return membershipLabel;
    }
    const uint8_t busId = membership(instr);
    if (busId == kFxBusNone)
        snprintf(membershipLabel, sizeof(membershipLabel), "Bus Off");
    else
        snprintf(membershipLabel, sizeof(membershipLabel), "Bus %u", (unsigned)busId);
    return membershipLabel;
}

LabelState FxBuses::state() const
{
    const uint8_t instr = getEngineService().selectedSampleInstrument();
    if (instr == 0xFF)
        return LabelState::Empty;
    return membership(instr) != kFxBusNone ? LabelState::Active : LabelState::Filled;
}

bool FxBuses::onPress(PageHost& host, SoftKeyButtonState pressState, bool shift)
{
    (void)host;
    (void)shift;
    if (pressState != SoftKeyButtonState::Press)
        return true;
    const uint8_t instr = getEngineService().selectedSampleInstrument();
    if (instr == 0xFF)
        return true;
    uint8_t next = static_cast<uint8_t>(membership(instr) + 1);
    if (next > kFxBusCount)
        next = kFxBusNone;
    setMembership(instr, next);
    return true;
}

bool FxBuses::EditBusSoftKey::onPress(PageHost& host, SoftKeyButtonState pressState, bool shift)
{
    (void)shift;
    (void)host;
    if (pressState != SoftKeyButtonState::Press)
        return true;
    uint8_t busId = 1;
    const uint8_t instr = getEngineService().selectedSampleInstrument();
    if (instr != 0xFF)
    {
        busId = owner.membership(instr);
        if (busId == kFxBusNone)
            busId = 1;
    }
    // Picks the bus to edit. The Instrument screen opens pageToOpen() after this returns.
    owner.instrumentPage.prepareForOpen(busId);
    return true;
}

void FxBuses::EditorPage::prepareForOpen(uint8_t busId1Based)
{
    if (busId1Based < 1 || busId1Based > kFxBusCount)
        busId1Based = 1;
    editingBus = busId1Based;
    focusedParam = 0;
}

void FxBuses::EditorPage::onShow(PageHost& host)
{
    refreshEditorUi(host);
}

void FxBuses::EditorPage::onHide(PageHost& host)
{
    (void)host;
    owner.saveToWorkspace();
    owner.refreshMemberVoices(editingBus);
}

void FxBuses::EditorPage::selectBus(PageHost& host, uint8_t busId1Based)
{
    if (busId1Based < 1 || busId1Based > kFxBusCount)
        return;
    editingBus = busId1Based;
    focusedParam = 0;
    refreshEditorUi(host);
}

void FxBuses::EditorPage::refreshEditorUi(PageHost& host)
{
    strFxBus* b = owner.bus(editingBus);
    if (b == nullptr)
        return;

    // The screen adds its own page counter (Master shows "FX Buses 4/4"). The bus being
    // edited is the highlighted soft key.
    host.setTitle(title());

    host.setListCount(kParamCount);
    char valueBuf[12];
    for (uint8_t i = 0; i < kParamCount; i++)
    {
        switch (i)
        {
            case 0:
                snprintf(valueBuf, sizeof(valueBuf), "%s",
                         b->filterEnable ? (b->filterType == 0   ? "LP"
                                            : b->filterType == 1 ? "HP"
                                                                 : "BP")
                                         : "Off");
                break;
            case 1: snprintf(valueBuf, sizeof(valueBuf), "%d", (int)(b->cutOff * 100.0f)); break;
            case 2: snprintf(valueBuf, sizeof(valueBuf), "%d", (int)(b->resonance * 100.0f)); break;
            case 3: snprintf(valueBuf, sizeof(valueBuf), "%u", (unsigned)b->overdrive); break;
            case 4: snprintf(valueBuf, sizeof(valueBuf), "%u", (unsigned)b->bitDepth); break;
            case 5: snprintf(valueBuf, sizeof(valueBuf), "%u", (unsigned)b->delaySend); break;
            case 6: snprintf(valueBuf, sizeof(valueBuf), "%u", (unsigned)b->reverbSend); break;
            default: valueBuf[0] = '\0'; break;
        }
        host.setListRow(i, kParamNames[i], valueBuf);
    }
    host.setListFocus(focusedParam, true);

    for (uint8_t i = 0; i < 4; i++)
    {
        char label[8];
        snprintf(label, sizeof(label), "Bus %u", (unsigned)(i + 1));
        host.setSoftKey(i, label,
                        (i + 1 == editingBus) ? LabelState::Active : LabelState::Filled);
    }
    host.setSoftKey(4, "Prev", LabelState::Filled);
    host.setSoftKey(5, "Next", LabelState::Filled);
    host.setSoftKey(6, "", LabelState::Empty);
    host.setSoftKey(7, "Back", LabelState::Filled);
    host.finishSoftKeys();
    host.refresh();
}

void FxBuses::EditorPage::changeFocusedParam(PageHost& host, int8_t delta)
{
    strFxBus* b = owner.bus(editingBus);
    if (b == nullptr || delta == 0)
        return;

    switch (focusedParam)
    {
        case 0:
        {
            int mode = b->filterEnable ? (int)b->filterType + 1 : 0;
            mode += delta;
            if (mode < 0)
                mode = 3;
            if (mode > 3)
                mode = 0;
            if (mode == 0)
            {
                b->filterEnable = 0;
                b->filterType = 0;
            }
            else
            {
                b->filterEnable = 1;
                b->filterType = (uint8_t)(mode - 1);
            }
            break;
        }
        case 1:
        {
            float v = b->cutOff + delta * 0.01f;
            if (v < 0.0f)
                v = 0.0f;
            if (v > 1.0f)
                v = 1.0f;
            b->cutOff = v;
            break;
        }
        case 2:
        {
            float v = b->resonance + delta * 0.01f;
            if (v < 0.0f)
                v = 0.0f;
            if (v > 1.0f)
                v = 1.0f;
            b->resonance = v;
            break;
        }
        case 3:
        {
            int v = (int)b->overdrive + delta;
            if (v < 0)
                v = 0;
            if (v > 100)
                v = 100;
            b->overdrive = (uint8_t)v;
            break;
        }
        case 4:
        {
            int v = (int)b->bitDepth + delta;
            if (v < 1)
                v = 1;
            if (v > (int)kFxBusBitDepthMax)
                v = (int)kFxBusBitDepthMax;
            b->bitDepth = (uint8_t)v;
            break;
        }
        case 5:
        {
            int v = (int)b->delaySend + delta;
            if (v < 0)
                v = 0;
            if (v > (int)kFxBusSendMax)
                v = (int)kFxBusSendMax;
            b->delaySend = (uint8_t)v;
            break;
        }
        case 6:
        {
            int v = (int)b->reverbSend + delta;
            if (v < 0)
                v = 0;
            if (v > (int)kFxBusSendMax)
                v = (int)kFxBusSendMax;
            b->reverbSend = (uint8_t)v;
            break;
        }
        default: break;
    }
    owner.saveToWorkspace();
    owner.refreshMemberVoices(editingBus);
    refreshEditorUi(host);
}

void FxBuses::EditorPage::onInput(PageHost& host, const InputEvent& event)
{
    switch (event.kind)
    {
        case InputKind::SoftKey:
            if (event.state != SoftKeyButtonState::Press)
                return;
            if (event.index < 4)
            {
                selectBus(host, static_cast<uint8_t>(event.index + 1));
            }
            else if (event.index == 4)
            {
                if (focusedParam == 0)
                    focusedParam = kParamCount - 1;
                else
                    focusedParam--;
                refreshEditorUi(host);
            }
            else if (event.index == 5)
            {
                focusedParam = static_cast<uint8_t>((focusedParam + 1) % kParamCount);
                refreshEditorUi(host);
            }
            else if (event.index == 7)
            {
                host.close();
            }
            break;
        case InputKind::Encoder:
            changeFocusedParam(host, event.delta);
            break;
        case InputKind::Navigate:
            if (event.delta < 0)
            {
                if (focusedParam == 0)
                    focusedParam = kParamCount - 1;
                else
                    focusedParam--;
            }
            else
            {
                focusedParam = static_cast<uint8_t>((focusedParam + 1) % kParamCount);
            }
            refreshEditorUi(host);
            break;
        case InputKind::Back:
            host.close();
            break;
        default: break;
    }
}
