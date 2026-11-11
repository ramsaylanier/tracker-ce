#include "config.h"
#include "hostMasterVolume.h"

#include <atomic>
#include <stdint.h>

static bool s_audioMuted = false;
static std::atomic<float> s_hostMasterVolume{ 1.0f };

void disableAudioInterrupt()
{
    // No-op on host: main serializes with SDL_LockAudioDevice instead.
}

void enableAudioInterrupt() {}

void muteAudioOutByKey()
{
    s_audioMuted = true;
}

void unmuteAudioOutByKey()
{
    s_audioMuted = false;
}

uint8_t isAudioOutMuted()
{
    return s_audioMuted ? 1 : 0;
}

extern "C" void hostSetMasterVolumeGain(float volume)
{
    if (volume < 0.0f)
        volume = 0.0f;
    else if (volume > 1.0f)
        volume = 1.0f;
    s_hostMasterVolume.store(volume, std::memory_order_relaxed);
}

extern "C" float hostGetMasterVolumeGain(void)
{
    return s_hostMasterVolume.load(std::memory_order_relaxed);
}
