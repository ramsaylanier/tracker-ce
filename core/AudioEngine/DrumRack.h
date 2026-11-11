#ifndef AUDIOENGINE_DRUMRACK_H_
#define AUDIOENGINE_DRUMRACK_H_

#include "AudioStream.h"
#include "mtStructs.h"
#include <stdint.h>

class AudioDrumKitSend : public AudioStream
{
  public:
    static constexpr uint8_t kVoiceCount = 8;
    static constexpr uint8_t kNoKit = 0xFF;

    AudioDrumKitSend();

    void update() override;

    void clearVoice(uint8_t voiceIdx);
    // kitIdx is a drum-rack pool index (0..kDrumRackInstrumentsCount-1).
    void setVoiceKit(uint8_t voiceIdx, uint8_t kitIdx, uint8_t padIdx = 0);
    void setVoiceScale(uint8_t voiceIdx, float scale);
    // Per-voice wet gains (already include pad+bus send, voice scale, and FX volume).
    void setVoiceSendGains(uint8_t voiceIdx, float delayGain, float reverbGain);
    void clearKitSendGains(uint8_t kitIdx);

    uint8_t getVoiceKit(uint8_t voiceIdx) const;
    uint8_t getVoicePad(uint8_t voiceIdx) const;

  private:
    audio_block_t* inputQueueArray[kVoiceCount];
    uint8_t voiceKitIdx[kVoiceCount];
    uint8_t voicePadIdx[kVoiceCount];
    float voiceScale[kVoiceCount];
    float voiceDelayGain[kVoiceCount];
    float voiceReverbGain[kVoiceCount];
};

namespace DrumRack
{
void initProjectRacks();
void clearPadReferencesToSlot(uint8_t sampleSlot);
void clearLegacySampleSlotRacks(class FileSystem* fileSystem);
bool saveSidecar(uint8_t poolIdx, class FileSystem* fileSystem);
bool loadSidecar(uint8_t poolIdx, class FileSystem* fileSystem);
void chokeOtherPads(uint8_t kitPoolIdx, uint8_t padIdx, uint8_t sourceTrack);
bool kitHasOtherActiveVoices(uint8_t kitPoolIdx, uint8_t sourceTrack);
AudioDrumKitSend& sendBus();

int findFirstInactivePoolIdx();
bool activatePoolSlot(uint8_t poolIdx);
void deactivatePoolSlot(uint8_t poolIdx, class FileSystem* fileSystem);

inline bool isGlobalDrumRack(uint8_t globalIdx)
{
    return globalIdx >= DRUM_RACK_INSTRUMENTS_START_IDX &&
           globalIdx < DRUM_RACK_INSTRUMENTS_START_IDX + kDrumRackInstrumentsCount;
}

inline uint8_t poolIdxFromGlobal(uint8_t globalIdx)
{
    return (uint8_t)(globalIdx - DRUM_RACK_INSTRUMENTS_START_IDX);
}

inline strDrumRackInstrument* poolInstrument(uint8_t poolIdx)
{
    if (poolIdx >= kDrumRackInstrumentsCount)
        return nullptr;
    return &mtProject.drumRackInstrument[poolIdx];
}

inline strDrumRackInstrument* globalInstrument(uint8_t globalIdx)
{
    if (!isGlobalDrumRack(globalIdx))
        return nullptr;
    return &mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)];
}

inline uint8_t firstNoteOfGlobal(uint8_t globalIdx)
{
    strDrumRackInstrument* instr = globalInstrument(globalIdx);
    if (instr == nullptr)
        return kDrumRackFirstNote;
    return instr->rack.firstNote;
}

inline uint8_t firstNoteOfPool(uint8_t poolIdx)
{
    strDrumRackInstrument* instr = poolInstrument(poolIdx);
    if (instr == nullptr)
        return kDrumRackFirstNote;
    return instr->rack.firstNote;
}

inline bool isVoiceOnGlobalKit(uint8_t voice, uint8_t globalInstrIdx)
{
    return isGlobalDrumRack(globalInstrIdx) &&
           sendBus().getVoiceKit(voice) == poolIdxFromGlobal(globalInstrIdx);
}

// Resolve kit-bus / sample-instrument fields for a global instrument index.
inline uint8_t busFilterEnable(uint8_t globalIdx)
{
    if (isGlobalDrumRack(globalIdx))
        return mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)].filterEnable;
    return mtProject.instrument[globalIdx].filterEnable;
}
inline uint8_t busFilterType(uint8_t globalIdx)
{
    if (isGlobalDrumRack(globalIdx))
        return mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)].filterType;
    return mtProject.instrument[globalIdx].filterType;
}
inline float busCutOff(uint8_t globalIdx)
{
    if (isGlobalDrumRack(globalIdx))
        return mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)].cutOff;
    return mtProject.instrument[globalIdx].cutOff;
}
inline float busResonance(uint8_t globalIdx)
{
    if (isGlobalDrumRack(globalIdx))
        return mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)].resonance;
    return mtProject.instrument[globalIdx].resonance;
}
inline uint8_t busOverdrive(uint8_t globalIdx)
{
    if (isGlobalDrumRack(globalIdx))
        return mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)].overdrive;
    return mtProject.instrument[globalIdx].overdrive;
}
inline uint8_t busBitDepth(uint8_t globalIdx)
{
    if (isGlobalDrumRack(globalIdx))
        return mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)].bitDepth;
    return mtProject.instrument[globalIdx].bitDepth;
}
inline uint8_t busDelaySend(uint8_t globalIdx)
{
    if (isGlobalDrumRack(globalIdx))
        return mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)].delaySend;
    return mtProject.instrument[globalIdx].delaySend;
}
inline uint8_t busReverbSend(uint8_t globalIdx)
{
    if (isGlobalDrumRack(globalIdx))
        return mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)].reverbSend;
    return mtProject.instrument[globalIdx].reverbSend;
}

// Effective kit send for a pad: min(100, padSend + busSend).
inline uint8_t effectivePadDelaySend(uint8_t globalIdx, uint8_t padIdx)
{
    if (!isGlobalDrumRack(globalIdx) || padIdx >= kDrumRackPadCount)
        return 0;
    const strDrumRackInstrument& instr = mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)];
    const int sum = (int)instr.rack.delaySend[padIdx] + (int)instr.delaySend;
    return (uint8_t)(sum > 100 ? 100 : sum);
}

inline uint8_t effectivePadReverbSend(uint8_t globalIdx, uint8_t padIdx)
{
    if (!isGlobalDrumRack(globalIdx) || padIdx >= kDrumRackPadCount)
        return 0;
    const strDrumRackInstrument& instr = mtProject.drumRackInstrument[poolIdxFromGlobal(globalIdx)];
    const int sum = (int)instr.rack.reverbSend[padIdx] + (int)instr.reverbSend;
    return (uint8_t)(sum > 100 ? 100 : sum);
}

inline strDrumRackPad* editPad(uint8_t globalIdx)
{
    strDrumRackInstrument* instr = globalInstrument(globalIdx);
    if (instr == nullptr || !instr->isActive)
        return nullptr;
    if (instr->rack.selectedPad >= kDrumRackPadCount)
        instr->rack.selectedPad = 0;
    return &instr->rack.pads[instr->rack.selectedPad];
}

inline void selectPad(uint8_t globalIdx, uint8_t padIdx)
{
    strDrumRackInstrument* instr = globalInstrument(globalIdx);
    if (instr == nullptr || padIdx >= kDrumRackPadCount)
        return;
    instr->rack.selectedPad = padIdx;
}

inline void cycleSelectedPad(uint8_t globalIdx, int8_t delta)
{
    strDrumRackInstrument* instr = globalInstrument(globalIdx);
    if (instr == nullptr)
        return;
    int next = (int)instr->rack.selectedPad + (int)delta;
    while (next < 0)
        next += kDrumRackPadCount;
    while (next >= (int)kDrumRackPadCount)
        next -= kDrumRackPadCount;
    instr->rack.selectedPad = (uint8_t)next;
}

inline void cyclePadSampleSlot(uint8_t globalIdx, int8_t delta)
{
    strDrumRackPad* pad = editPad(globalIdx);
    if (pad == nullptr)
        return;

    int slot = (pad->sampleSlot == kDrumRackEmptySlot) ? -1 : (int)pad->sampleSlot;
    for (int attempt = 0; attempt < (int)kSampleInstrumentsCount + 1; attempt++)
    {
        slot += (int)delta;
        if (slot < -1)
            slot = (int)kSampleInstrumentsCount - 1;
        if (slot >= (int)kSampleInstrumentsCount)
            slot = -1;
        if (slot < 0)
        {
            pad->sampleSlot = kDrumRackEmptySlot;
            return;
        }
        if (mtProject.instrument[slot].isActive == 1)
        {
            pad->sampleSlot = (uint8_t)slot;
            return;
        }
        if (delta == 0)
            break;
    }
    pad->sampleSlot = kDrumRackEmptySlot;
}
} // namespace DrumRack

#endif /* AUDIOENGINE_DRUMRACK_H_ */
