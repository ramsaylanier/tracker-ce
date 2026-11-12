#include "PatternLaunchScenes.h"

#include "ModuleSidecar.h"
#include "Platform.h"

#include <cr_section_macros.h>
#include <cstdio>
#include <cstring>

__NOINIT(EXTERNAL_RAM) static strPatternScenesFile externalRamPatternScenesBank;

PatternLaunchScenes patternLaunchScenes;

void PatternLaunchScenes::init(FileSystem* fileSystem)
{
    this->fileSystem = fileSystem;
    clearBank();
}

void PatternLaunchScenes::onWorkspaceClear()
{
    clearBank();
}

void PatternLaunchScenes::onWorkspaceLoad()
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

void PatternLaunchScenes::onProjectOpen(const char* projectPath, const char* projectName)
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

void PatternLaunchScenes::onProjectSave(const char* projectPath, const char* projectName)
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

void PatternLaunchScenes::clearBank()
{
    memset(&externalRamPatternScenesBank, 0, sizeof(externalRamPatternScenesBank));
    externalRamPatternScenesBank.magic[0] = 'P';
    externalRamPatternScenesBank.magic[1] = 'T';
    externalRamPatternScenesBank.magic[2] = 'S';
    externalRamPatternScenesBank.magic[3] = 'C';
    externalRamPatternScenesBank.version = kPatternScenesFileVersion;
    externalRamPatternScenesBank.count = kPatternScenesCount;
    activeSlot = 0xFF;
}

void PatternLaunchScenes::setActiveSlot(uint8_t idx)
{
    activeSlot = (idx < kPatternScenesCount) ? idx : 0xFF;
}

void PatternLaunchScenes::refreshPage(PageHost& host)
{
    static const char* const slotLabels[kPatternScenesCount] = {
        "1", "2", "3", "4", "5", "6", "7", "8"
    };

    if (activeSlot < kPatternScenesCount)
    {
        // TitleBar stores a pointer — keep this buffer stable.
        static char titleBuf[24];
        snprintf(titleBuf, sizeof(titleBuf), "Patterns - %u", static_cast<unsigned>(activeSlot + 1));
        host.setTitle(titleBuf);
    }
    else
    {
        host.setTitle(title());
    }

    for (uint8_t i = 0; i < kPatternScenesCount; i++)
    {
        LabelState state = LabelState::Empty;
        if (i == activeSlot)
        {
            state = LabelState::Active;
        }
        else if (isFilled(i))
        {
            state = LabelState::Filled;
        }
        host.setSoftKey(i, slotLabels[i], state);
    }
    host.finishSoftKeys();
}

bool PatternLaunchScenes::validateHeader() const
{
    return externalRamPatternScenesBank.magic[0] == 'P' &&
           externalRamPatternScenesBank.magic[1] == 'T' &&
           externalRamPatternScenesBank.magic[2] == 'S' &&
           externalRamPatternScenesBank.magic[3] == 'C' &&
           externalRamPatternScenesBank.version == kPatternScenesFileVersion &&
           externalRamPatternScenesBank.count == kPatternScenesCount;
}

bool PatternLaunchScenes::readFile(const char* path)
{
    if (!ModuleSidecar::read(
          fileSystem, path, &externalRamPatternScenesBank, sizeof(externalRamPatternScenesBank)))
    {
        return false;
    }

    if (!validateHeader())
    {
        clearBank();
        return false;
    }

    return true;
}

void PatternLaunchScenes::writeFile(const char* path)
{
    externalRamPatternScenesBank.magic[0] = 'P';
    externalRamPatternScenesBank.magic[1] = 'T';
    externalRamPatternScenesBank.magic[2] = 'S';
    externalRamPatternScenesBank.magic[3] = 'C';
    externalRamPatternScenesBank.version = kPatternScenesFileVersion;
    externalRamPatternScenesBank.count = kPatternScenesCount;

    ModuleSidecar::write(
      fileSystem, path, &externalRamPatternScenesBank, sizeof(externalRamPatternScenesBank));
}

void PatternLaunchScenes::saveToWorkspace()
{
    char path[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::workspacePath(path, sizeof(path), id());
    writeFile(path);
}

bool PatternLaunchScenes::isFilled(uint8_t idx) const
{
    if (idx >= kPatternScenesCount)
    {
        return false;
    }
    return externalRamPatternScenesBank.scenes[idx].name[0] != '\0';
}

void PatternLaunchScenes::storeScene(uint8_t idx, const char* name)
{
    if (idx >= kPatternScenesCount)
    {
        return;
    }

    strPatternScene& scene = externalRamPatternScenesBank.scenes[idx];
    getPerformanceService().capturePatternScene(scene);

    memset(scene.name, 0, sizeof(scene.name));
    if (name != nullptr)
    {
        strncpy(scene.name, name, kPerfFxSceneNameLength - 1);
    }

    saveToWorkspace();
}

bool PatternLaunchScenes::recallScene(uint8_t idx)
{
    if (idx >= kPatternScenesCount || !isFilled(idx))
    {
        return false;
    }

    getPerformanceService().recallPatternScene(externalRamPatternScenesBank.scenes[idx]);
    setActiveSlot(idx);
    return true;
}

void PatternLaunchScenes::onShow(PageHost& host)
{
    refreshPage(host);
}

void PatternLaunchScenes::onHide(PageHost& host)
{
    (void)host;
}

void PatternLaunchScenes::onInput(PageHost& host, const InputEvent& event)
{
    if (event.kind != InputKind::SoftKey)
    {
        return;
    }

    if (event.state != SoftKeyButtonState::Press || event.index >= kPatternScenesCount)
    {
        return;
    }

    if (event.shift || host.isShiftPressed())
    {
        char name[kPerfFxSceneNameLength];
        snprintf(name, sizeof(name), "Pat %d", event.index + 1);
        storeScene(event.index, name);
        refreshPage(host);
    }
    else if (recallScene(event.index))
    {
        refreshPage(host);
    }
}
