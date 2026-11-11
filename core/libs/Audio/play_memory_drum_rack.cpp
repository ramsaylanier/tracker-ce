#include "Audio.h"
#include "DrumRack.h"
#include "play_memory.h"
#include "utility/dspinst.h"

AUDIO_CORE_FLASH void AudioPlayMemory::playDrumRack(uint8_t kitPoolIdx, int8_t note)
{
    AudioNoInterrupts();

    if (kitPoolIdx >= kDrumRackInstrumentsCount)
    {
        AudioInterrupts();
        return;
    }

    const uint8_t padIdx =
      drumRackNoteToPad(note, DrumRack::firstNoteOfPool(kitPoolIdx));
    const strDrumRackPad& pad =
      mtProject.drumRackInstrument[kitPoolIdx].rack.pads[padIdx];

    if (pad.sampleSlot == kDrumRackEmptySlot || pad.sampleSlot >= kSampleInstrumentsCount ||
        mtProject.instrument[pad.sampleSlot].isActive != 1 ||
        mtProject.instrument[pad.sampleSlot].sample.length == 0 ||
        mtProject.sampleMemory[pad.sampleSlot].address[channel] == nullptr)
    {
        AudioInterrupts();
        return;
    }

    const uint8_t sampleSlot = pad.sampleSlot;
    // Pitch is rooted at C4; pad tune/finetune replace the kit instrument's.
    const int8_t playNote = 60;

    lastInstrIdxInPlay = currentInstrIdxInPlay;
    currentInstrIdxInPlay = sampleSlot;
    currentInstrIdx = sampleSlot;

    if (playing == 1)
        needSmoothingFlag = 1;
    playing = 0;
    loopBackwardFlag = 0;
    iPitchCounter = 0;
    fPitchCounter = 0;
    pitchSmoothingControlCounter = 0;

    currentGlide = 0;
    currentTune = pad.tune + mtProject.instrument[sampleSlot].tuneResample;
    currentFineTune = pad.fineTune;
    currentStartAddress = mtProject.sampleMemory[sampleSlot].address[channel];
    currentPlayMode = playModeSingleShot;
    currentSampleLength = mtProject.instrument[sampleSlot].sample.length;
    currentIsStreamed = false;
    currentPreCacheFrames = 0;
    currentSliceCacheAddress = nullptr;
    currentSliceCacheFrames = 0;
    if (channel == 0 && voiceIndex >= 0)
        voiceStreamManager.notifyNoteOff((uint8_t)voiceIndex);

    constrainCurrentTune(playNote);
    setStartPitch(playNote);
    applyFinetuneOnPitch(playNote);
    calculateGlidePitch(playNote);
    lastNote = playNote;

    const uint16_t startPoint = mtProject.instrument[sampleSlot].startPoint;
    const uint16_t endPoint = mtProject.instrument[sampleSlot].endPoint;
    if (startPoint >= endPoint)
    {
        AudioInterrupts();
        return;
    }

    pointsInSamples.start = calculatePointPositionAndSnapToZero(
      startPoint, (uint32_t)currentSampleLength, (int16_t*)currentStartAddress);
    pointsInSamples.end = calculatePointPositionAndSnapToZero(
      endPoint, (uint32_t)currentSampleLength, (int16_t*)currentStartAddress);
    pointsInSamples.loop1 = 0;
    pointsInSamples.loop2 = 0;

    if ((pointsInSamples.start >= currentSampleLength) ||
        (pointsInSamples.end > currentSampleLength))
    {
        AudioInterrupts();
        return;
    }

    constrainsInSamples.endPoint = pointsInSamples.end - pointsInSamples.start;
    next = currentStartAddress + pointsInSamples.start;
    length = currentSampleLength - pointsInSamples.start;
    iPitchCounter = reverseDirectionFlag ? constrainsInSamples.endPoint - 1 : 0;

    playing = 1;
    AudioInterrupts();
}
