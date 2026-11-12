#ifndef FX_BUSES_H_
#define FX_BUSES_H_

#include "FeatureModule.h"

#include <stdint.h>

static constexpr uint8_t kFxBusCount = 4;
static constexpr uint8_t kFxBusNone = 0; // membership: 0 = off, 1..kFxBusCount = bus
static constexpr uint16_t kFxBusesFileVersion = 1;
static constexpr uint8_t kFxBusNameSize = 16;
static constexpr uint8_t kFxBusSendMax = 100;
static constexpr uint8_t kFxBusBitDepthMax = 16;

struct strFxBus
{
    char name[kFxBusNameSize];
    float cutOff;
    float resonance;
    uint8_t filterType;
    uint8_t filterEnable;
    uint8_t overdrive;
    uint8_t bitDepth;
    uint8_t delaySend;
    uint8_t reverbSend;
    uint8_t _pad[2];
};

struct strFxBusesFile
{
    char magic[4];
    uint16_t version;
    uint16_t busCount;
    uint8_t instrumentFxBus[EngineService::kSampleInstrumentCount];
    uint8_t _pad[2];
    strFxBus buses[kFxBusCount];
};

static_assert(sizeof(strFxBusesFile) % 4 == 0,
              "strFxBusesFile must be 4-byte aligned for FatFs DMA");

class FxBuses : public FeatureModule, public SoftKeyHandler
{
  public:
    // Per-screen editor UI so Master and Instrument keep independent focus/bus state.
    class EditorPage : public ModulePage
    {
      public:
        explicit EditorPage(FxBuses& owner_)
          : owner(owner_)
        {
        }

        const char* title() const override { return "FX Buses"; }
        void onShow(PageHost& host) override;
        void onHide(PageHost& host) override;
        void onInput(PageHost& host, const InputEvent& event) override;

        void prepareForOpen(uint8_t busId1Based);

      private:
        void refreshEditorUi(PageHost& host);
        void changeFocusedParam(PageHost& host, int8_t delta);
        void selectBus(PageHost& host, uint8_t busId1Based);

        FxBuses& owner;
        uint8_t editingBus = 1; // 1..kFxBusCount
        uint8_t focusedParam = 0;
    };

    FxBuses();
    const char* id() const override { return "fxbuses"; }

    void registerExtensions(ExtensionRegistrar& registrar) override;

    void init(FileSystem* fileSystem) override;
    void onWorkspaceClear() override;
    void onWorkspaceLoad() override;
    void onProjectOpen(const char* projectPath, const char* projectName) override;
    void onProjectSave(const char* projectPath, const char* projectName) override;

    // Instrument Effects softkey (slot 5): cycle bus membership
    const char* label() const override;
    LabelState state() const override;
    bool onPress(PageHost& host, SoftKeyButtonState state, bool shift) override;

    uint8_t membership(uint8_t sampleInstrIdx) const;
    void setMembership(uint8_t sampleInstrIdx, uint8_t busId);
    strFxBus* bus(uint8_t busId1Based);
    const strFxBus* bus(uint8_t busId1Based) const;
    strFxBus* busForInstrument(uint8_t sampleInstrIdx);
    const strFxBus* busForInstrument(uint8_t sampleInstrIdx) const;

    void initDefaultBus(strFxBus& bus, uint8_t index0Based);
    void saveToWorkspace();
    void refreshMemberVoices(uint8_t busId1Based);

    EditorPage& instrumentEditor() { return instrumentPage; }
    EditorPage& masterEditor() { return masterPage; }

  private:
    class EditBusSoftKey : public SoftKeyHandler
    {
      public:
        explicit EditBusSoftKey(FxBuses& owner_)
          : owner(owner_)
        {
        }
        const char* label() const override { return "Edit Bus"; }
        LabelState state() const override { return LabelState::Filled; }
        bool onPress(PageHost& host, SoftKeyButtonState state, bool shift) override;
        ModulePage* pageToOpen() const override { return &owner.instrumentPage; }

      private:
        FxBuses& owner;
    };

    void clearBank();
    bool readFile(const char* path);
    void writeFile(const char* path);
    bool validateHeader() const;

    FileSystem* fileSystem = nullptr;
    EditBusSoftKey editBusSoftKey;
    EditorPage masterPage;
    EditorPage instrumentPage;
    mutable char membershipLabel[12] = {};
};

extern FxBuses fxBuses;

#endif /* FX_BUSES_H_ */
