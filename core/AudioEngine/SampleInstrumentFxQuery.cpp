#include "SampleInstrumentFxQuery.h"

#include "mtStructs.h"

extern strMtProject mtProject;

static const SampleInstrumentFxProvider* gSampleInstrumentFxProvider = nullptr;

void setSampleInstrumentFxProvider(const SampleInstrumentFxProvider* provider)
{
    gSampleInstrumentFxProvider = provider;
}

namespace SampleInstrumentFx
{
uint8_t filterEnable(uint8_t sampleInstrIdx)
{
    if (gSampleInstrumentFxProvider != nullptr && gSampleInstrumentFxProvider->filterEnable != nullptr)
        return gSampleInstrumentFxProvider->filterEnable(sampleInstrIdx);
    if (sampleInstrIdx >= kSampleInstrumentsCount)
        return 0;
    return mtProject.instrument[sampleInstrIdx].filterEnable;
}

uint8_t filterType(uint8_t sampleInstrIdx)
{
    if (gSampleInstrumentFxProvider != nullptr && gSampleInstrumentFxProvider->filterType != nullptr)
        return gSampleInstrumentFxProvider->filterType(sampleInstrIdx);
    if (sampleInstrIdx >= kSampleInstrumentsCount)
        return 0;
    return mtProject.instrument[sampleInstrIdx].filterType;
}

float cutOff(uint8_t sampleInstrIdx)
{
    if (gSampleInstrumentFxProvider != nullptr && gSampleInstrumentFxProvider->cutOff != nullptr)
        return gSampleInstrumentFxProvider->cutOff(sampleInstrIdx);
    if (sampleInstrIdx >= kSampleInstrumentsCount)
        return 1.0f;
    return mtProject.instrument[sampleInstrIdx].cutOff;
}

float resonance(uint8_t sampleInstrIdx)
{
    if (gSampleInstrumentFxProvider != nullptr && gSampleInstrumentFxProvider->resonance != nullptr)
        return gSampleInstrumentFxProvider->resonance(sampleInstrIdx);
    if (sampleInstrIdx >= kSampleInstrumentsCount)
        return 0.0f;
    return mtProject.instrument[sampleInstrIdx].resonance;
}

uint8_t overdrive(uint8_t sampleInstrIdx)
{
    if (gSampleInstrumentFxProvider != nullptr && gSampleInstrumentFxProvider->overdrive != nullptr)
        return gSampleInstrumentFxProvider->overdrive(sampleInstrIdx);
    if (sampleInstrIdx >= kSampleInstrumentsCount)
        return 0;
    return mtProject.instrument[sampleInstrIdx].overdrive;
}

uint8_t bitDepth(uint8_t sampleInstrIdx)
{
    if (gSampleInstrumentFxProvider != nullptr && gSampleInstrumentFxProvider->bitDepth != nullptr)
        return gSampleInstrumentFxProvider->bitDepth(sampleInstrIdx);
    if (sampleInstrIdx >= kSampleInstrumentsCount)
        return BIT_DEPTH_MAX;
    return mtProject.instrument[sampleInstrIdx].bitDepth;
}

uint8_t effectiveDelaySend(uint8_t sampleInstrIdx)
{
    if (gSampleInstrumentFxProvider != nullptr &&
        gSampleInstrumentFxProvider->effectiveDelaySend != nullptr)
        return gSampleInstrumentFxProvider->effectiveDelaySend(sampleInstrIdx);
    if (sampleInstrIdx >= kSampleInstrumentsCount)
        return 0;
    return mtProject.instrument[sampleInstrIdx].delaySend;
}

uint8_t effectiveReverbSend(uint8_t sampleInstrIdx)
{
    if (gSampleInstrumentFxProvider != nullptr &&
        gSampleInstrumentFxProvider->effectiveReverbSend != nullptr)
        return gSampleInstrumentFxProvider->effectiveReverbSend(sampleInstrIdx);
    if (sampleInstrIdx >= kSampleInstrumentsCount)
        return 0;
    return mtProject.instrument[sampleInstrIdx].reverbSend;
}
} // namespace SampleInstrumentFx
