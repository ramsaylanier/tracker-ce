#ifndef MODULES_SDK_SCENES_H_
#define MODULES_SDK_SCENES_H_

#include <stdint.h>

// Snapshot sizes match firmware track counts in mtStructs.h
// (kSampleTracks=8, kMidiTracks=8, kPerformanceTracks=16).
static constexpr uint8_t kPerfHostSampleTracks = 8;
static constexpr uint8_t kPerfHostMidiTracks = 8;
static constexpr uint8_t kPerfHostPerformanceTracks = 16;
static constexpr uint8_t kPerfFxSceneSlotCount = 12;
static constexpr uint8_t kPerfFxSceneNameLength = 16;

struct strPerfScene
{
    char name[kPerfFxSceneNameLength];
    uint8_t fxPlaces[kPerfFxSceneSlotCount];
    uint8_t selectedValues[kPerfFxSceneSlotCount];
    int16_t fxValues[kPerfFxSceneSlotCount][4];
    int16_t liveValues[kPerfFxSceneSlotCount];
};

static_assert(sizeof(strPerfScene) % 4 == 0, "strPerfScene must be 4-byte aligned for FatFs DMA");

struct strPatternScene
{
    char name[kPerfFxSceneNameLength];
    uint16_t trackPatterns[kPerfHostPerformanceTracks];
};

static_assert(sizeof(strPatternScene) % 4 == 0,
              "strPatternScene must be 4-byte aligned for FatFs DMA");

struct strMuteScene
{
    char name[kPerfFxSceneNameLength];
    uint8_t sampleMute[kPerfHostSampleTracks];
    uint8_t midiMute[kPerfHostMidiTracks];
};

static_assert(sizeof(strMuteScene) % 4 == 0, "strMuteScene must be 4-byte aligned for FatFs DMA");

#endif /* MODULES_SDK_SCENES_H_ */
