#include "SongRowMutes.h"

#include "ModuleSidecar.h"
#include "Platform.h"
#include "SongRowMuteQuery.h"
#include "sdk/Services.h"

#include <cr_section_macros.h>
#include <cstring>

namespace
{
uint8_t minU8(uint16_t a, uint16_t b)
{
    return static_cast<uint8_t>(a < b ? a : b);
}
} // namespace

__NOINIT(EXTERNAL_RAM) static strSongRowMutesFile externalRamSongRowMutesBank;

SongRowMutes songRowMutes;

static bool songRowMutesIsMuted(uint8_t slot, uint8_t track)
{
    return songRowMutes.isMuted(slot, track);
}

static bool songRowMutesToggleSelection(uint8_t startSlot,
                                        uint8_t slotCount,
                                        uint8_t startTrack,
                                        uint8_t trackCount)
{
    return songRowMutes.toggleSelection(startSlot, slotCount, startTrack, trackCount);
}

static const SongRowMuteProvider kSongRowMuteProvider = {
    &songRowMutesIsMuted,
    &songRowMutesToggleSelection,
};

void SongRowMutes::init(FileSystem* fileSystem)
{
    this->fileSystem = fileSystem;
    clearBank();
    setSongRowMuteProvider(&kSongRowMuteProvider);
}

void SongRowMutes::onWorkspaceClear()
{
    clearBank();
}

void SongRowMutes::onWorkspaceLoad()
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

void SongRowMutes::onProjectOpen(const char* projectPath, const char* projectName)
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

void SongRowMutes::onProjectSave(const char* projectPath, const char* projectName)
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

void SongRowMutes::clearBank()
{
    memset(&externalRamSongRowMutesBank, 0, sizeof(externalRamSongRowMutesBank));
    externalRamSongRowMutesBank.magic[0] = 'S';
    externalRamSongRowMutesBank.magic[1] = 'R';
    externalRamSongRowMutesBank.magic[2] = 'M';
    externalRamSongRowMutesBank.magic[3] = 'T';
    externalRamSongRowMutesBank.version = kSongRowMutesFileVersion;
    externalRamSongRowMutesBank.reserved = 0;
}

bool SongRowMutes::validateHeader() const
{
    return externalRamSongRowMutesBank.magic[0] == 'S' &&
           externalRamSongRowMutesBank.magic[1] == 'R' &&
           externalRamSongRowMutesBank.magic[2] == 'M' &&
           externalRamSongRowMutesBank.magic[3] == 'T' &&
           externalRamSongRowMutesBank.version == kSongRowMutesFileVersion;
}

bool SongRowMutes::readFile(const char* path)
{
    if (!ModuleSidecar::read(
          fileSystem, path, &externalRamSongRowMutesBank, sizeof(externalRamSongRowMutesBank)))
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

void SongRowMutes::writeFile(const char* path)
{
    externalRamSongRowMutesBank.magic[0] = 'S';
    externalRamSongRowMutesBank.magic[1] = 'R';
    externalRamSongRowMutesBank.magic[2] = 'M';
    externalRamSongRowMutesBank.magic[3] = 'T';
    externalRamSongRowMutesBank.version = kSongRowMutesFileVersion;
    externalRamSongRowMutesBank.reserved = 0;

    ModuleSidecar::write(
      fileSystem, path, &externalRamSongRowMutesBank, sizeof(externalRamSongRowMutesBank));
}

void SongRowMutes::saveToWorkspace()
{
    char path[ModuleSidecar::kMaxPathLength];
    ModuleSidecar::workspacePath(path, sizeof(path), id());
    writeFile(path);
}

bool SongRowMutes::isMuted(uint8_t slot, uint8_t track) const
{
    if (slot >= kSongRowMutesSlotCount)
    {
        return false;
    }

    if (track < kSongRowMutesSampleTracks)
    {
        return externalRamSongRowMutesBank.sampleMute[slot][track] != 0;
    }

    const uint8_t midiTrack = static_cast<uint8_t>(track - kSongRowMutesSampleTracks);
    if (midiTrack < kSongRowMutesMidiTracks)
    {
        return externalRamSongRowMutesBank.midiMute[slot][midiTrack] != 0;
    }

    return false;
}

void SongRowMutes::setMuted(uint8_t slot, uint8_t track, bool muted)
{
    if (slot >= kSongRowMutesSlotCount)
    {
        return;
    }

    const uint8_t value = muted ? 1 : 0;
    if (track < kSongRowMutesSampleTracks)
    {
        externalRamSongRowMutesBank.sampleMute[slot][track] = value;
        return;
    }

    const uint8_t midiTrack = static_cast<uint8_t>(track - kSongRowMutesSampleTracks);
    if (midiTrack < kSongRowMutesMidiTracks)
    {
        externalRamSongRowMutesBank.midiMute[slot][midiTrack] = value;
    }
}

bool SongRowMutes::toggleSelection(uint8_t startSlot,
                                   uint8_t slotCount,
                                   uint8_t startTrack,
                                   uint8_t trackCount)
{
    if (slotCount == 0 || trackCount == 0 || startSlot >= kSongRowMutesSlotCount)
    {
        return false;
    }

    const uint8_t endSlotExclusive = minU8(startSlot + slotCount, kSongRowMutesSlotCount);
    const uint8_t endTrackExclusive = minU8(startTrack + trackCount, 16);

    bool changed = false;
    for (uint8_t slot = startSlot; slot < endSlotExclusive; slot++)
    {
        for (uint8_t track = startTrack; track < endTrackExclusive; track++)
        {
            setMuted(slot, track, !isMuted(slot, track));
            changed = true;
        }
    }

    if (changed)
    {
        saveToWorkspace();
    }
    return changed;
}

void SongRowMutes::clearSlot(uint8_t slot)
{
    if (slot >= kSongRowMutesSlotCount)
    {
        return;
    }
    memset(externalRamSongRowMutesBank.sampleMute[slot],
           0,
           sizeof(externalRamSongRowMutesBank.sampleMute[slot]));
    memset(externalRamSongRowMutesBank.midiMute[slot],
           0,
           sizeof(externalRamSongRowMutesBank.midiMute[slot]));
}

void SongRowMutes::applySlot(uint8_t slot)
{
    strMuteScene scene = {};
    if (slot < kSongRowMutesSlotCount)
    {
        memcpy(scene.sampleMute,
               externalRamSongRowMutesBank.sampleMute[slot],
               sizeof(scene.sampleMute));
        memcpy(scene.midiMute, externalRamSongRowMutesBank.midiMute[slot], sizeof(scene.midiMute));
    }
    getPerformanceService().recallMuteScene(scene);
}

void SongRowMutes::onSongSlotStart(uint8_t slot)
{
    applySlot(slot);
}

void SongRowMutes::onSongPlaylistReplaced()
{
    clearBank();
    saveToWorkspace();
}

void SongRowMutes::onSongInserted(uint8_t index, bool duplicate)
{
    if (index >= kSongRowMutesSlotCount - 1)
    {
        return;
    }

    for (int i = kSongRowMutesSlotCount - 1; i > index; i--)
    {
        memcpy(externalRamSongRowMutesBank.sampleMute[i],
               externalRamSongRowMutesBank.sampleMute[i - 1],
               sizeof(externalRamSongRowMutesBank.sampleMute[i]));
        memcpy(externalRamSongRowMutesBank.midiMute[i],
               externalRamSongRowMutesBank.midiMute[i - 1],
               sizeof(externalRamSongRowMutesBank.midiMute[i]));
    }

    const uint8_t insertAt = static_cast<uint8_t>(index + 1);
    if (duplicate)
    {
        memcpy(externalRamSongRowMutesBank.sampleMute[insertAt],
               externalRamSongRowMutesBank.sampleMute[index],
               sizeof(externalRamSongRowMutesBank.sampleMute[insertAt]));
        memcpy(externalRamSongRowMutesBank.midiMute[insertAt],
               externalRamSongRowMutesBank.midiMute[index],
               sizeof(externalRamSongRowMutesBank.midiMute[insertAt]));
    }
    else
    {
        clearSlot(insertAt);
    }

    saveToWorkspace();
}

void SongRowMutes::onSongRemoved(uint8_t index)
{
    if (index >= kSongRowMutesSlotCount)
    {
        return;
    }

    for (uint8_t i = index; i < kSongRowMutesSlotCount - 1; i++)
    {
        memcpy(externalRamSongRowMutesBank.sampleMute[i],
               externalRamSongRowMutesBank.sampleMute[i + 1],
               sizeof(externalRamSongRowMutesBank.sampleMute[i]));
        memcpy(externalRamSongRowMutesBank.midiMute[i],
               externalRamSongRowMutesBank.midiMute[i + 1],
               sizeof(externalRamSongRowMutesBank.midiMute[i]));
    }
    clearSlot(kSongRowMutesSlotCount - 1);
    saveToWorkspace();
}
