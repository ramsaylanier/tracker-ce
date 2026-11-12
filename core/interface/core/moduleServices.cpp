#include "sdk/Services.h"

#include "core/interfaceDefs.h"
#include "mtAudioEngine.h"
#include "mtSequencer.h"
#include "mtStructs.h"
#include "performanceMode/performanceEngine.h"
#include "performanceMode/performanceMode.h"
#include "songEditor/songEditor.h"

#include <cstring>

static_assert(EngineService::kSampleInstrumentCount == kSampleInstrumentsCount,
              "EngineService::kSampleInstrumentCount must match kSampleInstrumentsCount");

class PerformanceServiceImpl : public PerformanceService
{
  public:
    void captureFxScene(strPerfScene& scene) override { performance.captureScene(scene); }
    void recallFxScene(const strPerfScene& scene) override { performance.recallScene(scene); }

    void capturePatternScene(strPatternScene& scene) override
    {
        memset(scene.name, 0, sizeof(scene.name));
        for (uint8_t track = 0; track < kPerfHostPerformanceTracks; track++)
        {
            if (track < kPerformanceTracks)
            {
                scene.trackPatterns[track] = mtProject.perfTracksPatterns[track];
            }
            else
            {
                scene.trackPatterns[track] = 1;
            }
        }
    }

    void recallPatternScene(const strPatternScene& scene) override
    {
        for (uint8_t track = 0; track < kPerformanceTracks && track < kPerfHostPerformanceTracks;
             track++)
        {
            uint16_t pattern = scene.trackPatterns[track];
            if (pattern < 1)
            {
                pattern = 1;
            }
            mtProject.perfTracksPatterns[track] = pattern;
            sequencer.setTrackToLoadNow(track, pattern);
        }
        performanceMode.refreshTrackPattern = 1;
    }

    void captureMuteScene(strMuteScene& scene) override
    {
        memset(scene.name, 0, sizeof(scene.name));
        for (uint8_t i = 0; i < kPerfHostSampleTracks && i < kSampleTracks; i++)
        {
            scene.sampleMute[i] = mtProject.values.trackMute[i] ? 1 : 0;
        }
        for (uint8_t i = 0; i < kPerfHostMidiTracks && i < kMaxMidiTracksInTrackers; i++)
        {
            scene.midiMute[i] = mtProject.values.midiTrackMute[i] ? 1 : 0;
        }
    }

    void recallMuteScene(const strMuteScene& scene) override
    {
        for (uint8_t i = 0; i < kSampleTracks && i < kPerfHostSampleTracks; i++)
        {
            uint8_t muted = scene.sampleMute[i] ? 1 : 0;
            mtProject.values.trackMute[i] = muted;
            engine.muteTrack(i, muted);
        }
        for (uint8_t i = 0; i < kMaxMidiTracksInTrackers && i < kPerfHostMidiTracks; i++)
        {
            mtProject.values.midiTrackMute[i] = scene.midiMute[i] ? 1 : 0;
        }
        performanceMode.refreshTrackState = 1;
    }

    uint8_t performanceFxCount() const override { return performance.getFxCount(); }
    const char* performanceFxName(uint8_t fxId) const override
    {
        return performance.getFxName(fxId);
    }
    int16_t performanceFxMin(uint8_t fxId) const override { return performance.getFxMin(fxId); }
    int16_t performanceFxMax(uint8_t fxId) const override { return performance.getFxMax(fxId); }
    void applyPerformanceFx(uint8_t fxId, int16_t value) override
    {
        performance.applyFx(fxId, value);
    }
    void clearPerformanceFx(uint8_t fxId) override { performance.clearFx(fxId); }

    void refreshFxRack() override
    {
        for (uint8_t place = 0; place < 12; place++)
        {
            performanceMode.showFxNames(place);
            performanceMode.showPerformaceValue(place);
        }
        performanceMode.lightUpPadBoard();
    }

    void restoreTrackLabels() override
    {
        performanceMode.titleBar->setTitle("Performance FX");
        performanceMode.showTracksPatterns();
        performanceMode.showTracksState(
          performanceMode.buttons->isButtonPressed(interfaceButtonShift));
    }

    uint8_t tracksPage() const override { return performanceMode.tracksPage ? 1 : 0; }

    bool isSampleTrackMuted(uint8_t index) const override
    {
        if (index >= kSampleTracks || index >= kPerfHostSampleTracks)
        {
            return false;
        }
        return mtProject.values.trackMute[index] != 0;
    }

    bool isMidiTrackMuted(uint8_t index) const override
    {
        if (index >= kMaxMidiTracksInTrackers || index >= kPerfHostMidiTracks)
        {
            return false;
        }
        return mtProject.values.midiTrackMute[index] != 0;
    }
};

class SongServiceImpl : public SongService
{
  public:
    uint8_t selectedSlot() const override { return songEditor.selectedSongPosition; }
};

class EngineServiceImpl : public EngineService
{
  public:
    uint8_t selectedSampleInstrument() const override
    {
        if (mtProject.values.lastUsedInstrument >= kSampleInstrumentsCount)
        {
            return 0xFF;
        }
        return mtProject.values.lastUsedInstrument;
    }

    uint8_t filterEnable(uint8_t sampleInstrIdx) const override
    {
        if (sampleInstrIdx >= kSampleInstrumentsCount)
        {
            return 0;
        }
        return mtProject.instrument[sampleInstrIdx].filterEnable;
    }
    uint8_t filterType(uint8_t sampleInstrIdx) const override
    {
        if (sampleInstrIdx >= kSampleInstrumentsCount)
        {
            return 0;
        }
        return mtProject.instrument[sampleInstrIdx].filterType;
    }
    float cutOff(uint8_t sampleInstrIdx) const override
    {
        if (sampleInstrIdx >= kSampleInstrumentsCount)
        {
            return 0.0f;
        }
        return mtProject.instrument[sampleInstrIdx].cutOff;
    }
    float resonance(uint8_t sampleInstrIdx) const override
    {
        if (sampleInstrIdx >= kSampleInstrumentsCount)
        {
            return 0.0f;
        }
        return mtProject.instrument[sampleInstrIdx].resonance;
    }
    uint8_t overdrive(uint8_t sampleInstrIdx) const override
    {
        if (sampleInstrIdx >= kSampleInstrumentsCount)
        {
            return 0;
        }
        return mtProject.instrument[sampleInstrIdx].overdrive;
    }
    uint8_t bitDepth(uint8_t sampleInstrIdx) const override
    {
        if (sampleInstrIdx >= kSampleInstrumentsCount)
        {
            return 0;
        }
        return mtProject.instrument[sampleInstrIdx].bitDepth;
    }
    uint8_t delaySend(uint8_t sampleInstrIdx) const override
    {
        if (sampleInstrIdx >= kSampleInstrumentsCount)
        {
            return 0;
        }
        return mtProject.instrument[sampleInstrIdx].delaySend;
    }
    uint8_t reverbSend(uint8_t sampleInstrIdx) const override
    {
        if (sampleInstrIdx >= kSampleInstrumentsCount)
        {
            return 0;
        }
        return mtProject.instrument[sampleInstrIdx].reverbSend;
    }

    void notifyInstrumentFxDirty(uint8_t sampleInstrIdx, uint32_t mask) override
    {
        if (sampleInstrIdx < kSampleInstrumentsCount)
        {
            engine.setRefreshBitForInstrument(sampleInstrIdx, mask);
        }
    }
};

static PerformanceServiceImpl s_performanceService;
static SongServiceImpl s_songService;
static EngineServiceImpl s_engineService;

PerformanceService& getPerformanceService()
{
    return s_performanceService;
}
SongService& getSongService()
{
    return s_songService;
}
EngineService& getEngineService()
{
    return s_engineService;
}
