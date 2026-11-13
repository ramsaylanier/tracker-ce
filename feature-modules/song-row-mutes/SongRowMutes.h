#ifndef SONG_ROW_MUTES_H_
#define SONG_ROW_MUTES_H_

#include "FeatureModule.h"
#include "sdk/Columns.h"
#include "sdk/Scenes.h"

#include <stdint.h>

class FileSystem;

static constexpr uint16_t kSongRowMutesFileVersion = 1;
static constexpr uint8_t kSongRowMutesSampleTracks = kPerfHostSampleTracks;
static constexpr uint8_t kSongRowMutesMidiTracks = kPerfHostMidiTracks;
static constexpr uint8_t kSongRowMutesSlotCount = SongColumn::kSongSlotCount;

struct strSongRowMutesFile
{
    char magic[4];
    uint16_t version;
    uint16_t reserved;
    uint8_t sampleMute[kSongRowMutesSlotCount][kSongRowMutesSampleTracks];
    uint8_t midiMute[kSongRowMutesSlotCount][kSongRowMutesMidiTracks];
};

static_assert(sizeof(strSongRowMutesFile) % 4 == 0,
              "strSongRowMutesFile must be 4-byte aligned for FatFs DMA");
static_assert(SongColumn::kSongSlotCount == 255,
              "SongColumn::kSongSlotCount must match SONG_MAX");

// Per-playlist-row sample/MIDI mute masks applied on song slot start.
class SongRowMutes : public FeatureModule, public ModuleEventListener
{
  public:
    const char* id() const override { return "songrowmutes"; }

    void registerExtensions(ExtensionRegistrar& registrar) override
    {
        registrar.addEventListener(this);
    }

    void init(FileSystem* fileSystem) override;
    void onWorkspaceClear() override;
    void onWorkspaceLoad() override;
    void onProjectOpen(const char* projectPath, const char* projectName) override;
    void onProjectSave(const char* projectPath, const char* projectName) override;

    void onSongSlotStart(uint8_t slot) override;
    void onSongPlaylistReplaced() override;
    void onSongInserted(uint8_t index, bool duplicate) override;
    void onSongRemoved(uint8_t index) override;

    bool isMuted(uint8_t slot, uint8_t track) const;
    bool toggleSelection(uint8_t startSlot,
                         uint8_t slotCount,
                         uint8_t startTrack,
                         uint8_t trackCount);

  private:
    void clearBank();
    bool readFile(const char* path);
    void writeFile(const char* path);
    bool validateHeader() const;
    void saveToWorkspace();
    void applySlot(uint8_t slot);
    void clearSlot(uint8_t slot);
    void setMuted(uint8_t slot, uint8_t track, bool muted);

    FileSystem* fileSystem = nullptr;
};

extern SongRowMutes songRowMutes;

#endif /* SONG_ROW_MUTES_H_ */
