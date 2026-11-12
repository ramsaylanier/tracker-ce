#include "SongFxScenes.h"

#include "ModuleSidecar.h"
#include "Platform.h"

#include <cr_section_macros.h>
#include <cstdio>
#include <cstring>

__NOINIT(EXTERNAL_RAM) static strSongFxScenesFile externalRamSongFxScenesBank;

SongFxScenes songFxScenes;

static const PatternStepFxInfo kSongFxScenePatternStepFx = {
    kSongFxScenePatternStepFxId,
    'N',
    "FX Scene",
    "FX Scene",
    "Apply Song FX scene 1-8.",
    1,
    kSongFxScenesCount,
    1,
};

bool songFxScenesPatternStepFxHandler(uint8_t typeId, uint8_t value)
{
    return songFxScenes.onPatternStepFx(typeId, value);
}

void SongFxScenes::registerExtensions(ExtensionRegistrar& registrar)
{
    registrar.addPage(ScreenId::Song, this, 15);
    registrar.addSongColumn(this);
    registrar.addPatternStepFx(&kSongFxScenePatternStepFx, &songFxScenesPatternStepFxHandler);
}

bool SongFxScenes::onPatternStepFx(uint8_t typeId, uint8_t value)
{
    if (typeId != kSongFxScenePatternStepFxId)
    {
        return false;
    }

    if (value < 1 || value > kSongFxScenesCount)
    {
        return true;
    }

    applyScene(static_cast<uint8_t>(value - 1));
    return true;
}

void SongFxScenes::init(FileSystem* fileSystem)
{
    this->fileSystem = fileSystem;
    clearBank();
}

void SongFxScenes::onWorkspaceClear()
{
    clearBank();
}

void SongFxScenes::onWorkspaceLoad()
{
    char path[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::workspacePath(path, sizeof(path), id());

    if (!readFile(path))
    {
        clearBank();
        if (fileSystem != nullptr && fileSystem->exists(path))
        {
            fileSystem->remove(path);
        }
    }
}

void SongFxScenes::onProjectOpen(const char* projectPath, const char* projectName)
{
    char srcPath[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::projectPath(srcPath, sizeof(srcPath), projectPath, projectName, id());

    char workspacePathBuf[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::workspacePath(workspacePathBuf, sizeof(workspacePathBuf), id());

    if (!readFile(srcPath))
    {
        clearBank();
        if (fileSystem != nullptr && fileSystem->exists(workspacePathBuf))
        {
            fileSystem->remove(workspacePathBuf);
        }
        return;
    }

    writeFile(workspacePathBuf);
}

void SongFxScenes::onProjectSave(const char* projectPath, const char* projectName)
{
    if (fileSystem == nullptr)
    {
        return;
    }

    char workspacePathBuf[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::workspacePath(workspacePathBuf, sizeof(workspacePathBuf), id());

    if (!fileSystem->exists(workspacePathBuf))
    {
        return;
    }

    if (!readFile(workspacePathBuf))
    {
        return;
    }

    char destPath[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::projectPath(destPath, sizeof(destPath), projectPath, projectName, id());
    writeFile(destPath);
}

void SongFxScenes::clearBank()
{
    memset(&externalRamSongFxScenesBank, 0, sizeof(externalRamSongFxScenesBank));
    externalRamSongFxScenesBank.magic[0] = 'S';
    externalRamSongFxScenesBank.magic[1] = 'F';
    externalRamSongFxScenesBank.magic[2] = 'X';
    externalRamSongFxScenesBank.magic[3] = 'S';
    externalRamSongFxScenesBank.version = kSongFxScenesFileVersion;
    externalRamSongFxScenesBank.count = kSongFxScenesCount;
    editingScene = 0;
    focusedEntry = 0;
}

bool SongFxScenes::validateHeader() const
{
    return externalRamSongFxScenesBank.magic[0] == 'S' &&
           externalRamSongFxScenesBank.magic[1] == 'F' &&
           externalRamSongFxScenesBank.magic[2] == 'X' &&
           externalRamSongFxScenesBank.magic[3] == 'S' &&
           externalRamSongFxScenesBank.version == kSongFxScenesFileVersion &&
           externalRamSongFxScenesBank.count == kSongFxScenesCount;
}

void SongFxScenes::clampSongSlotScenes()
{
    for (uint8_t i = 0; i < SongColumn::kSongSlotCount; i++)
    {
        if (externalRamSongFxScenesBank.songSlotScene[i] > kSongFxScenesCount)
        {
            externalRamSongFxScenesBank.songSlotScene[i] = kSongFxScenesNone;
        }
    }
    externalRamSongFxScenesBank.songSlotScene[SongColumn::kSongSlotCount] = 0;
}

bool SongFxScenes::readFile(const char* path)
{
    if (!ModuleSidecar::read(
          fileSystem, path, &externalRamSongFxScenesBank, sizeof(externalRamSongFxScenesBank)))
    {
        return false;
    }

    if (!validateHeader())
    {
        clearBank();
        return false;
    }

    clampSongSlotScenes();
    return true;
}

void SongFxScenes::writeFile(const char* path)
{
    externalRamSongFxScenesBank.magic[0] = 'S';
    externalRamSongFxScenesBank.magic[1] = 'F';
    externalRamSongFxScenesBank.magic[2] = 'X';
    externalRamSongFxScenesBank.magic[3] = 'S';
    externalRamSongFxScenesBank.version = kSongFxScenesFileVersion;
    externalRamSongFxScenesBank.count = kSongFxScenesCount;
    clampSongSlotScenes();

    ModuleSidecar::write(
      fileSystem, path, &externalRamSongFxScenesBank, sizeof(externalRamSongFxScenesBank));
}

void SongFxScenes::saveToWorkspace()
{
    char path[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::workspacePath(path, sizeof(path), id());
    writeFile(path);
}

uint8_t SongFxScenes::sceneForSlot(uint8_t slot) const
{
    if (slot >= SongColumn::kSongSlotCount)
    {
        return kSongFxScenesNone;
    }
    return externalRamSongFxScenesBank.songSlotScene[slot];
}

void SongFxScenes::assignSlot(uint8_t slot, uint8_t scene)
{
    if (slot >= SongColumn::kSongSlotCount)
    {
        return;
    }

    if (scene > kSongFxScenesCount)
    {
        scene = kSongFxScenesNone;
    }

    if (externalRamSongFxScenesBank.songSlotScene[slot] == scene)
    {
        return;
    }

    externalRamSongFxScenesBank.songSlotScene[slot] = scene;
    saveToWorkspace();
}

void SongFxScenes::formatCell(uint8_t slot, char* out, uint8_t outSize) const
{
    if (out == nullptr || outSize == 0)
    {
        return;
    }

    out[0] = '\0';
    if (slot >= SongColumn::kSongSlotCount)
    {
        return;
    }

    const uint8_t value = sceneForSlot(slot);
    if (value == kSongFxScenesNone)
    {
        snprintf(out, outSize, "-");
        return;
    }

    snprintf(out, outSize, "%u", static_cast<unsigned>(value));
}

void SongFxScenes::onEncoder(PageHost& host, uint8_t slot, int8_t delta)
{
    (void)host;
    if (slot >= SongColumn::kSongSlotCount || delta == 0)
    {
        return;
    }

    int16_t value = sceneForSlot(slot);
    value += delta;

    const int16_t maxValue = kSongFxScenesCount;
    while (value < 0)
    {
        value += maxValue + 1;
    }
    while (value > maxValue)
    {
        value -= maxValue + 1;
    }

    assignSlot(slot, static_cast<uint8_t>(value));
}

void SongFxScenes::onClear(PageHost& host, uint8_t slot)
{
    (void)host;
    assignSlot(slot, kSongFxScenesNone);
}

void SongFxScenes::onInserted(uint8_t index, bool duplicate)
{
    if (index >= SongColumn::kSongSlotCount - 1)
    {
        return;
    }

    for (int i = SongColumn::kSongSlotCount - 1; i > index; i--)
    {
        externalRamSongFxScenesBank.songSlotScene[i] =
          externalRamSongFxScenesBank.songSlotScene[i - 1];
    }

    const uint8_t insertAt = static_cast<uint8_t>(index + 1);
    if (duplicate)
    {
        externalRamSongFxScenesBank.songSlotScene[insertAt] =
          externalRamSongFxScenesBank.songSlotScene[index];
    }
    else
    {
        externalRamSongFxScenesBank.songSlotScene[insertAt] = kSongFxScenesNone;
    }

    saveToWorkspace();
}

void SongFxScenes::onRemoved(uint8_t index)
{
    if (index >= SongColumn::kSongSlotCount)
    {
        return;
    }

    for (uint8_t i = index; i < SongColumn::kSongSlotCount - 1; i++)
    {
        externalRamSongFxScenesBank.songSlotScene[i] =
          externalRamSongFxScenesBank.songSlotScene[i + 1];
    }
    externalRamSongFxScenesBank.songSlotScene[SongColumn::kSongSlotCount - 1] =
      kSongFxScenesNone;

    saveToWorkspace();
}

void SongFxScenes::clearAll()
{
    memset(externalRamSongFxScenesBank.songSlotScene,
           0,
           sizeof(externalRamSongFxScenesBank.songSlotScene));
    saveToWorkspace();
}

void SongFxScenes::clearAllPerformanceFx()
{
    PerformanceService& perf = getPerformanceService();
    const uint8_t count = perf.performanceFxCount();
    for (uint8_t fxId = 1; fxId < count; fxId++)
    {
        perf.clearPerformanceFx(fxId);
    }
}

void SongFxScenes::applyScene(uint8_t sceneIndex)
{
    if (sceneIndex >= kSongFxScenesCount)
    {
        return;
    }

    // Replace prior row FX entirely — clear everything, then apply this scene.
    clearAllPerformanceFx();

    PerformanceService& perf = getPerformanceService();
    const strSongFxScene& scene = externalRamSongFxScenesBank.scenes[sceneIndex];
    for (uint8_t i = 0; i < kSongFxEntriesPerScene; i++)
    {
        const uint8_t fxId = scene.entries[i].fxId;
        if (fxId == 0 || fxId >= perf.performanceFxCount())
        {
            continue;
        }
        perf.applyPerformanceFx(fxId, scene.entries[i].value);
    }
}

void SongFxScenes::onSlotStart(uint8_t slot)
{
    const uint8_t scene = sceneForSlot(slot);
    if (scene == kSongFxScenesNone || scene > kSongFxScenesCount)
    {
        // Row with "-" still clears leftover FX from the previous scene.
        clearAllPerformanceFx();
        return;
    }
    applyScene(static_cast<uint8_t>(scene - 1));
}

void SongFxScenes::onShow(PageHost& host)
{
    const uint8_t scene = sceneForSlot(getSongService().selectedSlot());
    if (scene >= 1 && scene <= kSongFxScenesCount)
    {
        editingScene = static_cast<uint8_t>(scene - 1);
    }
    else if (editingScene >= kSongFxScenesCount)
    {
        editingScene = 0;
    }
    focusedEntry = 0;
    focusedField = 0;

    strSongFxScene& bankScene = externalRamSongFxScenesBank.scenes[editingScene];
    if (bankScene.name[0] == '\0')
    {
        snprintf(bankScene.name,
                 sizeof(bankScene.name),
                 "FX %u",
                 static_cast<unsigned>(editingScene + 1));
        saveToWorkspace();
    }

    refreshEditorUi(host);
}

void SongFxScenes::selectEditingScene(PageHost& host, uint8_t sceneIndex)
{
    if (sceneIndex >= kSongFxScenesCount)
    {
        return;
    }

    editingScene = sceneIndex;
    focusedEntry = 0;
    focusedField = 0;

    strSongFxScene& bankScene = externalRamSongFxScenesBank.scenes[editingScene];
    if (bankScene.name[0] == '\0')
    {
        snprintf(bankScene.name,
                 sizeof(bankScene.name),
                 "FX %u",
                 static_cast<unsigned>(editingScene + 1));
        saveToWorkspace();
    }

    refreshEditorUi(host);
}

void SongFxScenes::onHide(PageHost& host)
{
    (void)host;
}

void SongFxScenes::onInput(PageHost& host, const InputEvent& event)
{
    switch (event.kind)
    {
    case InputKind::SoftKey:
        if (event.state != SoftKeyButtonState::Press || event.index >= kSongFxScenesCount)
        {
            return;
        }
        selectEditingScene(host, event.index);
        break;

    case InputKind::Encoder:
        if (focusedField == 0)
        {
            changeFocusedFxType(host, event.delta);
        }
        else
        {
            changeFocusedFxValue(host, event.delta);
        }
        break;

    case InputKind::Navigate:
    {
        if (event.delta == 0)
        {
            return;
        }

        int16_t next = static_cast<int16_t>(focusedEntry) + event.delta;
        while (next < 0)
        {
            next += kSongFxEntriesPerScene;
        }
        while (next >= kSongFxEntriesPerScene)
        {
            next -= kSongFxEntriesPerScene;
        }
        focusedEntry = static_cast<uint8_t>(next);
        refreshEditorUi(host);
        break;
    }

    case InputKind::HNavigate:
        if (event.delta == 0)
        {
            return;
        }
        focusedField = (focusedField == 0) ? 1 : 0;
        refreshEditorUi(host);
        break;

    case InputKind::Clear:
    {
        if (focusedEntry >= kSongFxEntriesPerScene)
        {
            return;
        }

        strSongFxEntry& e =
          externalRamSongFxScenesBank.scenes[editingScene].entries[focusedEntry];
        e.fxId = 0;
        e.value = 0;
        saveToWorkspace();
        refreshEditorUi(host);
        break;
    }

    case InputKind::Back:
        (void)host;
        break;

    default:
        break;
    }
}

bool SongFxScenes::sceneHasEntries(uint8_t sceneIndex) const
{
    if (sceneIndex >= kSongFxScenesCount)
    {
        return false;
    }

    const strSongFxScene& scene = externalRamSongFxScenesBank.scenes[sceneIndex];
    for (uint8_t i = 0; i < kSongFxEntriesPerScene; i++)
    {
        if (scene.entries[i].fxId != 0)
        {
            return true;
        }
    }
    return false;
}

void SongFxScenes::formatEntryName(uint8_t entry, char* out, uint8_t outSize) const
{
    if (out == nullptr || outSize == 0 || entry >= kSongFxEntriesPerScene)
    {
        return;
    }

    const strSongFxEntry& e = externalRamSongFxScenesBank.scenes[editingScene].entries[entry];
    if (e.fxId == 0)
    {
        snprintf(out, outSize, "---");
        return;
    }

    const char* name = getPerformanceService().performanceFxName(e.fxId);
    snprintf(out, outSize, "%s", name != nullptr && name[0] != '\0' ? name : "?");
}

void SongFxScenes::formatEntryValue(uint8_t entry, char* out, uint8_t outSize) const
{
    if (out == nullptr || outSize == 0 || entry >= kSongFxEntriesPerScene)
    {
        return;
    }

    const strSongFxEntry& e = externalRamSongFxScenesBank.scenes[editingScene].entries[entry];
    if (e.fxId == 0)
    {
        snprintf(out, outSize, "--");
        return;
    }

    snprintf(out, outSize, "%d", static_cast<int>(e.value));
}

void SongFxScenes::refreshEditorUi(PageHost& host)
{
    static char titleBuf[24];
    snprintf(titleBuf,
             sizeof(titleBuf),
             "FX Scene %u",
             static_cast<unsigned>(editingScene + 1));
    host.setTitle(titleBuf);

    static const char* const sceneLabels[kSongFxScenesCount] = {
        "1", "2", "3", "4", "5", "6", "7", "8"
    };

    for (uint8_t i = 0; i < kSongFxScenesCount; i++)
    {
        LabelState state = LabelState::Empty;
        if (i == editingScene)
        {
            state = LabelState::Active;
        }
        else if (sceneHasEntries(i))
        {
            state = LabelState::Filled;
        }
        host.setSoftKey(i, sceneLabels[i], state, nullptr);
    }
    host.finishSoftKeys();

    static char nameBuf[kSongFxEntriesPerScene][24];
    static char valueBuf[kSongFxEntriesPerScene][12];
    host.setListCount(kSongFxEntriesPerScene);
    for (uint8_t i = 0; i < kSongFxEntriesPerScene; i++)
    {
        formatEntryName(i, nameBuf[i], sizeof(nameBuf[i]));
        formatEntryValue(i, valueBuf[i], sizeof(valueBuf[i]));
        host.setListRow(i, nameBuf[i], valueBuf[i]);
    }
    host.setListFocus(focusedEntry, focusedField != 0);
    host.refresh();
}

uint8_t SongFxScenes::nextFxId(uint8_t current, int8_t delta) const
{
    PerformanceService& host = getPerformanceService();
    const uint8_t count = host.performanceFxCount();
    // Skip fxId 0 (none); allow empty only via clear.
    const int16_t minId = 1;
    const int16_t maxId = static_cast<int16_t>(count > 0 ? count - 1 : 0);
    if (maxId < minId)
    {
        return current;
    }

    auto isNamed = [&](uint8_t id) {
        const char* name = host.performanceFxName(id);
        return name != nullptr && name[0] != '\0';
    };

    int16_t id = current;
    if (id < minId || id > maxId)
    {
        // First assignment from empty: find first named FX.
        for (int16_t i = minId; i <= maxId; i++)
        {
            if (isNamed(static_cast<uint8_t>(i)))
            {
                return static_cast<uint8_t>(i);
            }
        }
        return current;
    }

    if (delta == 0)
    {
        return current;
    }

    // Honor |delta| so +10/-10 skip that many named FX types.
    int16_t steps = delta > 0 ? delta : static_cast<int16_t>(-delta);
    int16_t found = 0;
    uint8_t lastNamed = current;
    const uint8_t dir = delta > 0 ? 1 : 0;
    for (uint16_t guard = 0; guard < static_cast<uint16_t>(count) * static_cast<uint16_t>(steps); guard++)
    {
        id += dir ? 1 : -1;
        if (id < minId)
        {
            id = maxId;
        }
        if (id > maxId)
        {
            id = minId;
        }
        if (isNamed(static_cast<uint8_t>(id)))
        {
            lastNamed = static_cast<uint8_t>(id);
            found++;
            if (found >= steps)
            {
                return lastNamed;
            }
        }
    }

    return lastNamed;
}

int16_t SongFxScenes::clampFxValue(uint8_t fxId, int16_t value) const
{
    PerformanceService& host = getPerformanceService();
    const int16_t minV = host.performanceFxMin(fxId);
    const int16_t maxV = host.performanceFxMax(fxId);
    if (value < minV)
    {
        return minV;
    }
    if (value > maxV)
    {
        return maxV;
    }
    return value;
}

void SongFxScenes::changeFocusedFxType(PageHost& host, int8_t delta)
{
    if (delta == 0 || focusedEntry >= kSongFxEntriesPerScene)
    {
        return;
    }

    strSongFxEntry& e = externalRamSongFxScenesBank.scenes[editingScene].entries[focusedEntry];
    if (e.fxId == 0)
    {
        e.fxId = nextFxId(0, 1);
        e.value = 0;
    }
    else
    {
        e.fxId = nextFxId(e.fxId, delta);
        e.value = clampFxValue(e.fxId, e.value);
    }

    saveToWorkspace();
    refreshEditorUi(host);
}

void SongFxScenes::changeFocusedFxValue(PageHost& host, int8_t delta)
{
    if (delta == 0 || focusedEntry >= kSongFxEntriesPerScene)
    {
        return;
    }

    strSongFxEntry& e = externalRamSongFxScenesBank.scenes[editingScene].entries[focusedEntry];
    if (e.fxId == 0)
    {
        // Empty row: +/- on value assigns an FX first, then stays on value.
        e.fxId = nextFxId(0, 1);
        e.value = 0;
        saveToWorkspace();
        refreshEditorUi(host);
        return;
    }

    e.value = clampFxValue(e.fxId, static_cast<int16_t>(e.value + delta));
    saveToWorkspace();
    refreshEditorUi(host);
}
