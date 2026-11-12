#include "ExampleSdkModule.h"

#include <cstdio>
#include <cstring>

ExampleSdkModule exampleSdkModule;

void ExampleSdkModule::registerExtensions(ExtensionRegistrar& registrar)
{
    registrar.addPage(ScreenId::Performance, this, 90);
    registrar.addPage(ScreenId::Pattern, &patternPage, 90);
    registrar.addSoftKey(ScreenId::Pattern, BuiltinPage::Pattern::Main, 7, this);
    // Intentionally skip addSongColumn / addPatternColumn by default so the
    // example does not collide with song-fx-scenes when both are linked.
    // A second Song/Pattern column fails (first-wins; capacity is 1).
    // Uncomment to exercise those hooks in isolation:
    // registrar.addSongColumn(this);
    // registrar.addPatternColumn(this);
    registrar.addConfigItem(this);
    registrar.addEventListener(this);
}

void ExampleSdkModule::init(FileSystem* fs)
{
    (void)fs;
    clearAll();
    memset(patternCells, 0, sizeof(patternCells));
}

void ExampleSdkModule::onShow(PageHost& host)
{
    host.setTitle("Example");
    for (uint8_t i = 0; i < 8; i++)
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "Ex %u", (unsigned)(i + 1));
        host.setSoftKey(i, buf, LabelState::Filled);
    }
    host.finishSoftKeys();
}

void ExampleSdkModule::onHide(PageHost& host)
{
    (void)host;
}

void ExampleSdkModule::onInput(PageHost& host, const InputEvent& event)
{
    if (event.kind == InputKind::Back ||
        (event.kind == InputKind::SoftKey && event.state == SoftKeyButtonState::Press &&
         event.index == 7))
    {
        host.close();
    }
}

void ExampleSdkModule::PatternPage::onShow(PageHost& host)
{
    host.setTitle("Example Pat");
    for (uint8_t i = 0; i < 7; i++)
        host.setSoftKey(i, "", LabelState::Empty);
    host.setSoftKey(7, "Back", LabelState::Filled);
    host.finishSoftKeys();
}

void ExampleSdkModule::PatternPage::onHide(PageHost& host)
{
    (void)host;
}

void ExampleSdkModule::PatternPage::onInput(PageHost& host, const InputEvent& event)
{
    if (event.kind == InputKind::Back ||
        (event.kind == InputKind::SoftKey && event.state == SoftKeyButtonState::Press &&
         event.index == 7))
    {
        host.close();
    }
}

bool ExampleSdkModule::onPress(PageHost& host, SoftKeyButtonState state, bool shift)
{
    (void)host;
    (void)shift;
    (void)state;
    // Softkey is registered so authors can see the hook; return false so Pattern keeps
    // the slot's builtin behavior until this demo does real work.
    return false;
}

void ExampleSdkModule::formatCell(uint8_t slot, char* out, uint8_t outSize) const
{
    if (out == nullptr || outSize == 0)
        return;
    if (slot > SongColumn::kSongSlotCount)
    {
        out[0] = '\0';
        return;
    }
    if (songCells[slot] == 0)
        out[0] = '\0';
    else
        snprintf(out, outSize, "%u", (unsigned)songCells[slot]);
}

void ExampleSdkModule::onEncoder(PageHost& host, uint8_t slot, int8_t delta)
{
    (void)host;
    if (slot > SongColumn::kSongSlotCount)
        return;
    int v = (int)songCells[slot] + delta;
    if (v < 0)
        v = 0;
    if (v > 99)
        v = 99;
    songCells[slot] = (uint8_t)v;
}

void ExampleSdkModule::onClear(PageHost& host, uint8_t slot)
{
    (void)host;
    if (slot <= SongColumn::kSongSlotCount)
        songCells[slot] = 0;
}

void ExampleSdkModule::onInserted(uint8_t index, bool duplicate)
{
    if (index > SongColumn::kSongSlotCount)
        return;
    for (uint8_t i = SongColumn::kSongSlotCount; i > index; i--)
        songCells[i] = songCells[i - 1];
    songCells[index] = duplicate && index > 0 ? songCells[index - 1] : 0;
}

void ExampleSdkModule::onRemoved(uint8_t index)
{
    if (index > SongColumn::kSongSlotCount)
        return;
    for (uint8_t i = index; i < SongColumn::kSongSlotCount; i++)
        songCells[i] = songCells[i + 1];
    songCells[SongColumn::kSongSlotCount] = 0;
}

void ExampleSdkModule::clearAll()
{
    memset(songCells, 0, sizeof(songCells));
}

void ExampleSdkModule::onSlotStart(uint8_t slot)
{
    (void)slot;
}

void ExampleSdkModule::formatCell(uint8_t track, uint16_t step, char* out, uint8_t outSize) const
{
    if (out == nullptr || outSize == 0)
        return;
    if (track >= kMaxTracks || step >= kMaxSteps || patternCells[track][step] == 0)
    {
        out[0] = '\0';
        return;
    }
    snprintf(out, outSize, "%u", (unsigned)patternCells[track][step]);
}

void ExampleSdkModule::onEdit(uint8_t track, uint16_t step, int8_t delta)
{
    if (track >= kMaxTracks || step >= kMaxSteps)
        return;
    int v = (int)patternCells[track][step] + delta;
    if (v < 0)
        v = 0;
    if (v > 99)
        v = 99;
    patternCells[track][step] = (uint8_t)v;
}

void ExampleSdkModule::onClear(uint8_t track, uint16_t step)
{
    if (track < kMaxTracks && step < kMaxSteps)
        patternCells[track][step] = 0;
}

void ExampleSdkModule::onStep(uint8_t track, uint16_t step)
{
    (void)track;
    (void)step;
}

void ExampleSdkModule::onInserted(uint8_t track, uint16_t step, bool duplicate)
{
    if (track >= kMaxTracks || step >= kMaxSteps)
        return;
    for (uint16_t s = kMaxSteps - 1; s > step; s--)
        patternCells[track][s] = patternCells[track][s - 1];
    patternCells[track][step] =
      duplicate && step > 0 ? patternCells[track][step - 1] : 0;
}

void ExampleSdkModule::onRemoved(uint8_t track, uint16_t step)
{
    if (track >= kMaxTracks || step >= kMaxSteps)
        return;
    for (uint16_t s = step; s + 1 < kMaxSteps; s++)
        patternCells[track][s] = patternCells[track][s + 1];
    patternCells[track][kMaxSteps - 1] = 0;
}

void ExampleSdkModule::onPatternLoad(uint16_t patternIdx)
{
    (void)patternIdx;
    memset(patternCells, 0, sizeof(patternCells));
}

void ExampleSdkModule::onPatternSave(uint16_t patternIdx)
{
    (void)patternIdx;
}

const char* const* ExampleSdkModule::listTexts() const
{
    static const char* const texts[2] = { "Off", "On" };
    return texts;
}

void ExampleSdkModule::onSongSlotStart(uint8_t slot)
{
    (void)slot;
}
