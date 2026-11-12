#ifndef EXAMPLE_SDK_MODULE_H_
#define EXAMPLE_SDK_MODULE_H_

#include "FeatureModule.h"

#include <stdint.h>

// Demonstrates every Module SDK v2 registration hook.
// Not linked by default (directory name starts with _).
class ExampleSdkModule : public FeatureModule,
                         public ModulePage,
                         public SoftKeyHandler,
                         public SongColumn,
                         public PatternColumn,
                         public ConfigItem,
                         public ModuleEventListener
{
  public:
    // Minimal Pattern ModulePage demo (title + softkeys + Back).
    class PatternPage : public ModulePage
    {
      public:
        const char* title() const override { return "Example Pat"; }
        void onShow(PageHost& host) override;
        void onHide(PageHost& host) override;
        void onInput(PageHost& host, const InputEvent& event) override;
    };

    const char* id() const override { return "examplesdk"; }

    void registerExtensions(ExtensionRegistrar& registrar) override;

    void init(FileSystem* fs) override;

    // ModulePage (Performance)
    const char* title() const override { return "Example"; }
    void onShow(PageHost& host) override;
    void onHide(PageHost& host) override;
    void onInput(PageHost& host, const InputEvent& event) override;

    // SoftKey (Pattern Main, slot 7)
    const char* label() const override { return "Ex"; }
    LabelState state() const override { return LabelState::Filled; }
    bool onPress(PageHost& host, SoftKeyButtonState state, bool shift) override;

    // SongColumn
    const char* header() const override { return "EX"; }
    const char* focusEncoderLabel() const override { return "Ex"; }
    void formatCell(uint8_t slot, char* out, uint8_t outSize) const override;
    void onEncoder(PageHost& host, uint8_t slot, int8_t delta) override;
    void onClear(PageHost& host, uint8_t slot) override;
    void onInserted(uint8_t index, bool duplicate) override;
    void onRemoved(uint8_t index) override;
    void clearAll() override;
    void onSlotStart(uint8_t slot) override;

    // PatternColumn
    char letter() const override { return 'E'; }
    void formatCell(uint8_t track, uint16_t step, char* out, uint8_t outSize) const override;
    void onEdit(uint8_t track, uint16_t step, int8_t delta) override;
    void onClear(uint8_t track, uint16_t step) override;
    void onStep(uint8_t track, uint16_t step) override;
    void onInserted(uint8_t track, uint16_t step, bool duplicate) override;
    void onRemoved(uint8_t track, uint16_t step) override;
    void onPatternLoad(uint16_t patternIdx) override;
    void onPatternSave(uint16_t patternIdx) override;

    // ConfigItem
    const char* name() const override { return "Example flag"; }
    const char* description() const override { return "SDK example ConfigItem (On/Off)."; }
    ConfigItemType type() const override { return ConfigItemType::ListText; }
    uint8_t* valuePtr() override { return &configFlag; }
    uint8_t listCount() const override { return 2; }
    const char* const* listTexts() const override;

    void onSongSlotStart(uint8_t slot) override;

  private:
    PatternPage patternPage;
    uint8_t songCells[SongColumn::kSongSlotCount + 1] = {};
    uint8_t patternCells[PatternColumn::kMaxTracks][PatternColumn::kMaxSteps] = {};
    uint8_t configFlag = 0;
};

extern ExampleSdkModule exampleSdkModule;

#endif /* EXAMPLE_SDK_MODULE_H_ */
