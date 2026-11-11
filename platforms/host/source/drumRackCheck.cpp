#include "DrumRack.h"
#include "PlatformImpl.h"
#include "config.h"
#include "mtSequencer.h"
#include "mtStructs.h"

#include <cstdio>
#include <cstring>

extern strMtProject mtProject;

namespace
{
int g_failures = 0;

void expect(bool cond, const char* msg)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", msg);
        ++g_failures;
    }
    else
    {
        std::fprintf(stderr, "ok: %s\n", msg);
    }
}
} // namespace

int runDrumRackHostCheck(PlatformImpl& platform)
{
    g_failures = 0;
    std::fprintf(stderr, "drum-rack-check: start\n");

    expect(AudioDrumKitSend::kVoiceCount == 8, "sample voice / kit bus inputs are 8");
    expect(kSampleTracks == 8, "sample tracks are 8");
    expect(kSynthMaxVoices == 8, "synth max voices constant is 8");
#ifdef FEATURE_SYNTH
    expect(kSynthParts == 3, "synth parts stay separate (3)");
#else
    expect(kSynthParts == 0, "host Mini build keeps synth parts disabled");
#endif
    expect(sizeof(strInstrument) == 372, "strInstrument stays 372 bytes");
    expect(playModeDrumRack == playModeGranular + 1, "playModeDrumRack kept for Option A detect");
    expect(kDrumRackInstrumentsCount == 8, "drum rack pool capacity is 8");
    expect(DRUM_RACK_INSTRUMENTS_START_IDX == 67, "drum racks start at global index 67");
    expect(kTotalInstrumentsCount == 75, "total instruments include rack pool");
    expect(isDrumRackInstrument(67) && isDrumRackInstrument(74), "pool globals map as drum racks");
    expect(!isDrumRackInstrument(66) && !isDrumRackInstrument(0), "non-rack indices stay non-rack");

    DrumRack::initProjectRacks();
    AudioDrumKitSend& bus = DrumRack::sendBus();

    constexpr uint8_t kKitPool = 3;
    constexpr uint8_t kKitGlobal = DRUM_RACK_INSTRUMENTS_START_IDX + kKitPool;
    constexpr uint8_t kKickPad = 0;
    constexpr uint8_t kSnarePad = 1;
    constexpr uint8_t kHatPad = 2;

    expect(DrumRack::activatePoolSlot(kKitPool), "activate pool slot");
    mtProject.drumRackInstrument[kKitPool].reverbSend = 40;
    mtProject.drumRackInstrument[kKitPool].delaySend = 0;
    mtProject.drumRackInstrument[kKitPool].rack.pads[kKickPad].sampleSlot = 0;
    mtProject.drumRackInstrument[kKitPool].rack.pads[kSnarePad].sampleSlot = 1;
    mtProject.drumRackInstrument[kKitPool].rack.pads[kHatPad].sampleSlot = 2;

    for (uint8_t v = 0; v < AudioDrumKitSend::kVoiceCount; v++)
        bus.clearVoice(v);

    bus.setVoiceKit(0, kKitPool, kKickPad);
    bus.setVoiceKit(1, kKitPool, kSnarePad);
    bus.setVoiceKit(2, kKitPool, kHatPad);
    bus.setVoiceScale(0, 1.0f);
    bus.setVoiceScale(1, 1.0f);
    bus.setVoiceScale(2, 1.0f);
    bus.setVoiceSendGains(0, 0.0f, 0.5f);
    bus.setVoiceSendGains(1, 0.0f, 0.5f);
    bus.setVoiceSendGains(2, 0.0f, 0.5f);

    mtProject.drumRackInstrument[kKitPool].rack.reverbSend[kKickPad] = 20;
    expect(DrumRack::effectivePadReverbSend(kKitGlobal, kKickPad) == 60,
           "pad+bus reverb sends add and clamp to 100");
    expect(DrumRack::effectivePadDelaySend(kKitGlobal, kKickPad) == 0,
           "pad delay defaults to bus-only when pad send is 0");

    // Sidecar layout: firstNote precedes pad-send arrays so short (pre-send) files keep it.
    expect(offsetof(strDrumRackInstrument, rack) + offsetof(strDrumRack, firstNote) + 1 <=
             sizeof(strDrumRackInstrument) - 2 * kDrumRackPadCount,
           "firstNote is before trailing pad delay/reverb arrays");

    expect(bus.getVoiceKit(0) == kKitPool && bus.getVoicePad(0) == kKickPad, "track 1 plays kick pad");
    expect(bus.getVoiceKit(1) == kKitPool && bus.getVoicePad(1) == kSnarePad, "track 2 plays snare pad");
    expect(bus.getVoiceKit(2) == kKitPool && bus.getVoicePad(2) == kHatPad, "track 3 plays hat pad");
    expect(DrumRack::kitHasOtherActiveVoices(kKitPool, 0), "kit has other active voices from track 1");
    expect(DrumRack::kitHasOtherActiveVoices(kKitPool, 1), "kit has other active voices from track 2");
    expect(drumRackIdxToInstrument(kKitPool) == kKitGlobal, "pool idx maps to global instrument");
    expect(drumRackInstrumentToIdx(kKitGlobal) == kKitPool, "global maps back to pool idx");

    // Mute track 2 (snare): scale drops out of the shared send sum.
    mtProject.values.trackMute[1] = 1;
    bus.setVoiceScale(1, 0.0f);
    expect(bus.getVoiceKit(1) == kKitPool, "muted track stays registered until note-off");
    expect(bus.getVoiceKit(0) == kKitPool && bus.getVoiceKit(2) == kKitPool,
           "kick and hat remain on shared kit bus after mute");

    bus.clearVoice(1);
    expect(bus.getVoiceKit(1) == AudioDrumKitSend::kNoKit, "cleared voice leaves the kit bus");
    expect(bus.getVoiceKit(0) == kKitPool && bus.getVoiceKit(2) == kKitPool,
           "remaining kit voices keep shared membership");
    expect(DrumRack::kitHasOtherActiveVoices(kKitPool, 0),
           "hat still counts as other active kit voice for track 1");

    // Normal instrument path clears kit membership.
    bus.setVoiceKit(3, kKitPool, kKickPad);
    bus.clearVoice(3);
    expect(bus.getVoiceKit(3) == AudioDrumKitSend::kNoKit, "normal instrument path clears kit membership");

    // Sidecar missing => load leaves defaults (inactive empty rack).
    mtProject.drumRackInstrument[5].rack.pads[0].sampleSlot = 7;
    mtProject.drumRackInstrument[5].isActive = 1;
    FileSystem* fs = platform.getFileSystem();
    expect(DrumRack::loadSidecar(5, fs), "missing sidecar load succeeds");
    expect(mtProject.drumRackInstrument[5].isActive == 0,
           "missing sidecar restores inactive default");
    expect(mtProject.drumRackInstrument[5].rack.pads[0].sampleSlot == kDrumRackEmptySlot,
           "missing sidecar restores default empty pads");

    // Option A drop: sample slot with playModeDrumRack is cleared.
    mtProject.instrument[2] = defaultInstrumentParams;
    mtProject.instrument[2].isActive = 1;
    mtProject.instrument[2].playMode = playModeDrumRack;
    DrumRack::clearLegacySampleSlotRacks(fs);
    expect(mtProject.instrument[2].isActive == 0, "Option A sample-slot rack cleared on load");
    expect(mtProject.instrument[2].playMode != playModeDrumRack,
           "Option A playModeDrumRack removed from sample slot");

    // Kit bus is sample-track sized; synth pool constants stay untouched by kit membership.
    expect(AudioDrumKitSend::kVoiceCount == kSampleTracks,
           "kit bus matches sample tracks, not synth parts");
    expect(kSynthMaxVoices == 8,
           "synth max voices remain a separate pool constant");

    if (g_failures == 0)
        std::fprintf(stderr, "drum-rack-check: all checks passed\n");
    else
        std::fprintf(stderr, "drum-rack-check: %d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
