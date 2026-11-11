#include "DrumRack.h"
#include "Platform.h"
#include "fileManagerDefs.h"
#include "mtAudioEngine.h"
#include "mtSequencer.h"
#include "mtStructs.h"
#include <cstddef>
#include <cstdio>
#include <cstring>

extern playerEngine instrumentPlayer[];
extern strMtProject mtProject;

AudioDrumKitSend::AudioDrumKitSend()
  : AudioStream(kVoiceCount, inputQueueArray)
{
    for (uint8_t i = 0; i < kVoiceCount; i++)
    {
        voiceKitIdx[i] = kNoKit;
        voicePadIdx[i] = 0;
        voiceScale[i] = 0.0f;
        voiceDelayGain[i] = 0.0f;
        voiceReverbGain[i] = 0.0f;
        inputQueueArray[i] = nullptr;
    }
}

void AudioDrumKitSend::clearVoice(uint8_t voiceIdx)
{
    if (voiceIdx >= kVoiceCount)
        return;
    voiceKitIdx[voiceIdx] = kNoKit;
    voicePadIdx[voiceIdx] = 0;
    voiceScale[voiceIdx] = 0.0f;
    voiceDelayGain[voiceIdx] = 0.0f;
    voiceReverbGain[voiceIdx] = 0.0f;
}

void AudioDrumKitSend::setVoiceKit(uint8_t voiceIdx, uint8_t kitIdx, uint8_t padIdx)
{
    if (voiceIdx >= kVoiceCount)
        return;
    voiceKitIdx[voiceIdx] = kitIdx;
    voicePadIdx[voiceIdx] = padIdx;
}

void AudioDrumKitSend::setVoiceScale(uint8_t voiceIdx, float scale)
{
    if (voiceIdx >= kVoiceCount)
        return;
    voiceScale[voiceIdx] = scale;
}

void AudioDrumKitSend::setVoiceSendGains(uint8_t voiceIdx, float delayGain, float reverbGain)
{
    if (voiceIdx >= kVoiceCount)
        return;
    voiceDelayGain[voiceIdx] = delayGain;
    voiceReverbGain[voiceIdx] = reverbGain;
}

void AudioDrumKitSend::clearKitSendGains(uint8_t kitIdx)
{
    if (kitIdx >= kDrumRackInstrumentsCount)
        return;
    for (uint8_t v = 0; v < kVoiceCount; v++)
    {
        if (voiceKitIdx[v] != kitIdx)
            continue;
        voiceDelayGain[v] = 0.0f;
        voiceReverbGain[v] = 0.0f;
        voiceScale[v] = 0.0f;
    }
}

uint8_t AudioDrumKitSend::getVoiceKit(uint8_t voiceIdx) const
{
    if (voiceIdx >= kVoiceCount)
        return kNoKit;
    return voiceKitIdx[voiceIdx];
}

uint8_t AudioDrumKitSend::getVoicePad(uint8_t voiceIdx) const
{
    if (voiceIdx >= kVoiceCount)
        return 0;
    return voicePadIdx[voiceIdx];
}

AUDIO_CORE_RAM void AudioDrumKitSend::update()
{
    audio_block_t* in[kVoiceCount];
    bool any = false;
    for (uint8_t v = 0; v < kVoiceCount; v++)
    {
        in[v] = receiveReadOnly(v);
        if (in[v] != nullptr && voiceKitIdx[v] != kNoKit &&
            (voiceDelayGain[v] != 0.0f || voiceReverbGain[v] != 0.0f))
            any = true;
    }

    if (!any)
    {
        for (uint8_t v = 0; v < kVoiceCount; v++)
        {
            if (in[v] != nullptr)
                release(in[v]);
        }
        return;
    }

    audio_block_t* delayOut = allocate();
    audio_block_t* reverbOut = allocate();
    if (delayOut == nullptr || reverbOut == nullptr)
    {
        if (delayOut != nullptr)
            release(delayOut);
        if (reverbOut != nullptr)
            release(reverbOut);
        for (uint8_t v = 0; v < kVoiceCount; v++)
        {
            if (in[v] != nullptr)
                release(in[v]);
        }
        return;
    }

    memset(delayOut->data, 0, sizeof(delayOut->data));
    memset(reverbOut->data, 0, sizeof(reverbOut->data));

    for (uint8_t v = 0; v < kVoiceCount; v++)
    {
        if (in[v] == nullptr || voiceKitIdx[v] == kNoKit)
            continue;
        const float dGain = voiceDelayGain[v];
        const float rGain = voiceReverbGain[v];
        if (dGain == 0.0f && rGain == 0.0f)
            continue;

        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++)
        {
            const float sample = (float)in[v]->data[i];
            if (dGain != 0.0f)
            {
                int32_t s = (int32_t)(sample * dGain);
                if (s > 32767)
                    s = 32767;
                else if (s < -32768)
                    s = -32768;
                int32_t mixed = (int32_t)delayOut->data[i] + s;
                if (mixed > 32767)
                    mixed = 32767;
                else if (mixed < -32768)
                    mixed = -32768;
                delayOut->data[i] = (int16_t)mixed;
            }
            if (rGain != 0.0f)
            {
                int32_t s = (int32_t)(sample * rGain);
                if (s > 32767)
                    s = 32767;
                else if (s < -32768)
                    s = -32768;
                int32_t mixed = (int32_t)reverbOut->data[i] + s;
                if (mixed > 32767)
                    mixed = 32767;
                else if (mixed < -32768)
                    mixed = -32768;
                reverbOut->data[i] = (int16_t)mixed;
            }
        }
    }

    transmit(delayOut, 0);
    transmit(reverbOut, 1);
    release(delayOut);
    release(reverbOut);

    for (uint8_t v = 0; v < kVoiceCount; v++)
    {
        if (in[v] != nullptr)
            release(in[v]);
    }
}

namespace DrumRack
{
static AudioDrumKitSend s_sendBus;

AudioDrumKitSend& sendBus()
{
    return s_sendBus;
}

void initProjectRacks()
{
    for (uint8_t i = 0; i < kDrumRackInstrumentsCount; i++)
        initDefaultDrumRackInstrument(&mtProject.drumRackInstrument[i]);
}

void clearPadReferencesToSlot(uint8_t sampleSlot)
{
    for (uint8_t kit = 0; kit < kDrumRackInstrumentsCount; kit++)
    {
        for (uint8_t pad = 0; pad < kDrumRackPadCount; pad++)
        {
            if (mtProject.drumRackInstrument[kit].rack.pads[pad].sampleSlot == sampleSlot)
                mtProject.drumRackInstrument[kit].rack.pads[pad].sampleSlot = kDrumRackEmptySlot;
        }
    }
}

void clearLegacySampleSlotRacks(FileSystem* fileSystem)
{
    for (uint8_t i = 0; i < kSampleInstrumentsCount; i++)
    {
        if (!isDrumRackPlayMode(mtProject.instrument[i].playMode))
            continue;

        mtProject.instrument[i] = defaultInstrumentParams;

        if (fileSystem != nullptr)
        {
            char legacyPath[PATCH_SIZE];
            snprintf(legacyPath,
                     sizeof(legacyPath),
                     "Workspace/instruments/instrument_%02d.rack",
                     i + 1);
            if (fileSystem->exists(legacyPath))
                fileSystem->remove(legacyPath);
        }
    }
}

int findFirstInactivePoolIdx()
{
    for (uint8_t i = 0; i < kDrumRackInstrumentsCount; i++)
    {
        if (!mtProject.drumRackInstrument[i].isActive)
            return (int)i;
    }
    return -1;
}

bool activatePoolSlot(uint8_t poolIdx)
{
    if (poolIdx >= kDrumRackInstrumentsCount)
        return false;
    strDrumRackInstrument& instr = mtProject.drumRackInstrument[poolIdx];
    initDefaultDrumRackInstrument(&instr);
    instr.isActive = 1;
    snprintf(instr.name, sizeof(instr.name), "Drum Rack %u", (unsigned)(poolIdx + 1));
    return true;
}

void deactivatePoolSlot(uint8_t poolIdx, FileSystem* fileSystem)
{
    if (poolIdx >= kDrumRackInstrumentsCount)
        return;
    initDefaultDrumRackInstrument(&mtProject.drumRackInstrument[poolIdx]);
    sendBus().clearKitSendGains(poolIdx);

    if (fileSystem != nullptr)
    {
        char path[PATCH_SIZE];
        snprintf(path, sizeof(path), cWorkspaceDrumRackFileFormat, poolIdx + 1);
        if (fileSystem->exists(path))
            fileSystem->remove(path);
    }
}

bool saveSidecar(uint8_t poolIdx, FileSystem* fileSystem)
{
    if (fileSystem == nullptr || poolIdx >= kDrumRackInstrumentsCount)
        return false;
    if (!mtProject.drumRackInstrument[poolIdx].isActive)
        return true;

    char path[PATCH_SIZE];
    snprintf(path, sizeof(path), cWorkspaceDrumRackFileFormat, poolIdx + 1);

    auto [isOpen, handle] =
      fileSystem->openFile(path, FileSystem::FileOpenMode::kWriteOpen);
    if (!isOpen || handle == nullptr)
        return false;

    const int32_t written = fileSystem->writeFile(
      &mtProject.drumRackInstrument[poolIdx], sizeof(strDrumRackInstrument), handle);
    fileSystem->closeFile(handle);
    return written == (int32_t)sizeof(strDrumRackInstrument);
}

bool loadSidecar(uint8_t poolIdx, FileSystem* fileSystem)
{
    if (fileSystem == nullptr || poolIdx >= kDrumRackInstrumentsCount)
        return false;

    initDefaultDrumRackInstrument(&mtProject.drumRackInstrument[poolIdx]);

    char path[PATCH_SIZE];
    snprintf(path, sizeof(path), cWorkspaceDrumRackFileFormat, poolIdx + 1);

    if (!fileSystem->exists(path))
        return true;

    auto [isOpen, handle] = fileSystem->openFile(path, FileSystem::FileOpenMode::kRead);
    if (!isOpen || handle == nullptr)
        return false;

    const size_t fileSize = fileSystem->getFileSize(handle);
    if (fileSize == 0)
    {
        fileSystem->closeFile(handle);
        return true;
    }

    strDrumRackInstrument& instr = mtProject.drumRackInstrument[poolIdx];
    const size_t toRead =
      fileSize < sizeof(strDrumRackInstrument) ? fileSize : sizeof(strDrumRackInstrument);
    const int32_t read = fileSystem->readFile(&instr, (uint32_t)toRead, handle);
    fileSystem->closeFile(handle);

    if (read <= 0)
    {
        initDefaultDrumRackInstrument(&instr);
        return true;
    }

    // Older sidecars may omit trailing fields. firstNote sits just before the pad-send
    // arrays — only reset it when the read did not reach firstNote itself.
    constexpr size_t kFirstNoteEndOffset =
      offsetof(strDrumRackInstrument, rack) + offsetof(strDrumRack, firstNote) + sizeof(uint8_t);
    if ((size_t)read < kFirstNoteEndOffset)
        instr.rack.firstNote = kDrumRackFirstNote;
    if (instr.rack.firstNote > 127 - (kDrumRackPadCount - 1))
        instr.rack.firstNote = kDrumRackFirstNote;

    for (uint8_t pad = 0; pad < kDrumRackPadCount; pad++)
    {
        uint8_t slot = instr.rack.pads[pad].sampleSlot;
        if (slot != kDrumRackEmptySlot && slot >= kSampleInstrumentsCount)
            instr.rack.pads[pad].sampleSlot = kDrumRackEmptySlot;
    }
    return true;
}

void chokeOtherPads(uint8_t kitPoolIdx, uint8_t padIdx, uint8_t sourceTrack)
{
    if (kitPoolIdx >= kDrumRackInstrumentsCount || padIdx >= kDrumRackPadCount)
        return;

    const uint8_t choke =
      mtProject.drumRackInstrument[kitPoolIdx].rack.pads[padIdx].chokeGroup;
    if (choke == 0)
        return;

    for (uint8_t track = 0; track < kSampleTracks; track++)
    {
        if (track == sourceTrack)
            continue;
        if (sendBus().getVoiceKit(track) != kitPoolIdx)
            continue;

        const uint8_t otherPad = sendBus().getVoicePad(track);
        if (otherPad < kDrumRackPadCount &&
            mtProject.drumRackInstrument[kitPoolIdx].rack.pads[otherPad].chokeGroup == choke)
            instrumentPlayer[track].noteOff(Sequencer::STEP_NOTE_OFF_CUT);
    }
}

bool kitHasOtherActiveVoices(uint8_t kitPoolIdx, uint8_t sourceTrack)
{
    if (kitPoolIdx >= kDrumRackInstrumentsCount)
        return false;
    for (uint8_t track = 0; track < kSampleTracks; track++)
    {
        if (track == sourceTrack)
            continue;
        if (sendBus().getVoiceKit(track) == kitPoolIdx)
            return true;
    }
    return false;
}

} // namespace DrumRack
