#include "SongRowMuteQuery.h"

static const SongRowMuteProvider* gSongRowMuteProvider = nullptr;

void setSongRowMuteProvider(const SongRowMuteProvider* provider)
{
    gSongRowMuteProvider = provider;
}

bool songRowMuteProviderActive()
{
    return gSongRowMuteProvider != nullptr && gSongRowMuteProvider->isMuted != nullptr;
}

namespace SongRowMute
{
bool isMuted(uint8_t slot, uint8_t track)
{
    if (gSongRowMuteProvider == nullptr || gSongRowMuteProvider->isMuted == nullptr)
    {
        return false;
    }
    return gSongRowMuteProvider->isMuted(slot, track);
}

bool toggleSelection(uint8_t startSlot,
                     uint8_t slotCount,
                     uint8_t startTrack,
                     uint8_t trackCount)
{
    if (gSongRowMuteProvider == nullptr || gSongRowMuteProvider->toggleSelection == nullptr)
    {
        return false;
    }
    return gSongRowMuteProvider->toggleSelection(startSlot, slotCount, startTrack, trackCount);
}
} // namespace SongRowMute
