#ifndef MODULES_SONGROWMUTEQUERY_H_
#define MODULES_SONGROWMUTEQUERY_H_

#include <stdint.h>

// Main-thread Song arrange per-row mute query/toggle.
// Optional provider (song-row-mutes module) may be installed at FeatureModule::init.
//
// track: 0..7 = sample tracks, 8..15 = MIDI tracks.
// Do not call setSongRowMuteProvider from the audio callback.

struct SongRowMuteProvider
{
    bool (*isMuted)(uint8_t slot, uint8_t track);
    bool (*toggleSelection)(uint8_t startSlot,
                            uint8_t slotCount,
                            uint8_t startTrack,
                            uint8_t trackCount);
};

void setSongRowMuteProvider(const SongRowMuteProvider* provider);
bool songRowMuteProviderActive();

namespace SongRowMute
{
bool isMuted(uint8_t slot, uint8_t track);
bool toggleSelection(uint8_t startSlot,
                     uint8_t slotCount,
                     uint8_t startTrack,
                     uint8_t trackCount);
} // namespace SongRowMute

#endif /* MODULES_SONGROWMUTEQUERY_H_ */
