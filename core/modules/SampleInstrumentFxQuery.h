#ifndef MODULES_SAMPLEINSTRUMENTFXQUERY_H_
#define MODULES_SAMPLEINSTRUMENTFXQUERY_H_

#include <stdint.h>

// Audio-safe read-only query for sample-instrument filter/drive/bit/sends.
// Default = strInstrument fields. Optional provider (e.g. FX Buses) may be
// installed at FeatureModule::init on the main thread only.
//
// Safe to call from the audio thread after init. Do not call setProvider
// from the audio callback.

struct SampleInstrumentFxProvider
{
    uint8_t (*filterEnable)(uint8_t sampleInstrIdx);
    uint8_t (*filterType)(uint8_t sampleInstrIdx);
    float (*cutOff)(uint8_t sampleInstrIdx);
    float (*resonance)(uint8_t sampleInstrIdx);
    uint8_t (*overdrive)(uint8_t sampleInstrIdx);
    uint8_t (*bitDepth)(uint8_t sampleInstrIdx);
    uint8_t (*effectiveDelaySend)(uint8_t sampleInstrIdx);
    uint8_t (*effectiveReverbSend)(uint8_t sampleInstrIdx);
};

void setSampleInstrumentFxProvider(const SampleInstrumentFxProvider* provider);

namespace SampleInstrumentFx
{
uint8_t filterEnable(uint8_t sampleInstrIdx);
uint8_t filterType(uint8_t sampleInstrIdx);
float cutOff(uint8_t sampleInstrIdx);
float resonance(uint8_t sampleInstrIdx);
uint8_t overdrive(uint8_t sampleInstrIdx);
uint8_t bitDepth(uint8_t sampleInstrIdx);
uint8_t effectiveDelaySend(uint8_t sampleInstrIdx);
uint8_t effectiveReverbSend(uint8_t sampleInstrIdx);
} // namespace SampleInstrumentFx

#endif /* MODULES_SAMPLEINSTRUMENTFXQUERY_H_ */
