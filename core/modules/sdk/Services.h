#ifndef MODULES_SDK_SERVICES_H_
#define MODULES_SDK_SERVICES_H_

#include "Scenes.h"

#include <stdint.h>

// Narrow facades so modules never include core internals.

class PerformanceService
{
  public:
    virtual ~PerformanceService() {}

    virtual void captureFxScene(strPerfScene& scene) = 0;
    virtual void recallFxScene(const strPerfScene& scene) = 0;
    virtual void capturePatternScene(strPatternScene& scene) = 0;
    virtual void recallPatternScene(const strPatternScene& scene) = 0;
    virtual void captureMuteScene(strMuteScene& scene) = 0;
    virtual void recallMuteScene(const strMuteScene& scene) = 0;

    virtual uint8_t performanceFxCount() const = 0;
    virtual const char* performanceFxName(uint8_t fxId) const = 0;
    virtual int16_t performanceFxMin(uint8_t fxId) const = 0;
    virtual int16_t performanceFxMax(uint8_t fxId) const = 0;
    virtual void applyPerformanceFx(uint8_t fxId, int16_t value) = 0;
    virtual void clearPerformanceFx(uint8_t fxId) = 0;

    virtual void refreshFxRack() = 0;
    virtual void restoreTrackLabels() = 0;

    virtual uint8_t tracksPage() const = 0;
    virtual bool isSampleTrackMuted(uint8_t index) const = 0;
    virtual bool isMidiTrackMuted(uint8_t index) const = 0;
};

class SongService
{
  public:
    virtual ~SongService() {}

    virtual uint8_t selectedSlot() const = 0;
};

class EngineService
{
  public:
    virtual ~EngineService() {}

    static constexpr uint8_t kSampleInstrumentCount = 48;

    // Currently selected sample instrument in the Instrument editor
    // (0..kSampleInstrumentCount-1), or 0xFF if a MIDI instrument is selected.
    virtual uint8_t selectedSampleInstrument() const = 0;

    // Read sample-instrument local FX fields (strInstrument).
    virtual uint8_t filterEnable(uint8_t sampleInstrIdx) const = 0;
    virtual uint8_t filterType(uint8_t sampleInstrIdx) const = 0;
    virtual float cutOff(uint8_t sampleInstrIdx) const = 0;
    virtual float resonance(uint8_t sampleInstrIdx) const = 0;
    virtual uint8_t overdrive(uint8_t sampleInstrIdx) const = 0;
    virtual uint8_t bitDepth(uint8_t sampleInstrIdx) const = 0;
    virtual uint8_t delaySend(uint8_t sampleInstrIdx) const = 0;
    virtual uint8_t reverbSend(uint8_t sampleInstrIdx) const = 0;

    // Ask the audio engine to refresh voices for an instrument after FX changes.
    // mask bits match audioEngine refresh masks (cutoff/resonance/drive/bit/sends).
    virtual void notifyInstrumentFxDirty(uint8_t sampleInstrIdx, uint32_t mask = 0xFFFFFFFFu) = 0;
};

PerformanceService& getPerformanceService();
SongService& getSongService();
EngineService& getEngineService();

#endif /* MODULES_SDK_SERVICES_H_ */
