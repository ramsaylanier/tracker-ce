#ifndef MUTE_BANKS_H_
#define MUTE_BANKS_H_

#include "FeatureModule.h"

#include <stdint.h>

class FileSystem;

static constexpr uint8_t kMuteBanksCount = 8;
static constexpr uint16_t kMuteBanksFileVersion = 1;

struct strMuteBanksFile
{
    char magic[4];
    uint16_t version;
    uint16_t count;
    strMuteScene scenes[kMuteBanksCount];
};

static_assert(sizeof(strMuteBanksFile) % 4 == 0,
              "strMuteBanksFile must be 4-byte aligned for FatFs DMA");

class MuteBanks : public FeatureModule, public ModulePage
{
  public:
    const char* id() const override { return "mutebanks"; }

    void registerExtensions(ExtensionRegistrar& registrar) override
    {
        registrar.addPage(ScreenId::Performance, this, 30);
    }

    void init(FileSystem* fileSystem) override;
    void onWorkspaceClear() override;
    void onWorkspaceLoad() override;
    void onProjectOpen(const char* projectPath, const char* projectName) override;
    void onProjectSave(const char* projectPath, const char* projectName) override;

    const char* title() const override { return "Mutes"; }
    void onShow(PageHost& host) override;
    void onHide(PageHost& host) override;
    void onInput(PageHost& host, const InputEvent& event) override;

    bool isFilled(uint8_t idx) const;

  private:
    void clearBank();
    bool readFile(const char* path);
    void writeFile(const char* path);
    bool validateHeader() const;
    void saveToWorkspace();
    void storeScene(uint8_t idx, const char* name);
    bool recallScene(uint8_t idx);
    void setActiveSlot(uint8_t idx);
    void refreshPage(PageHost& host);

    FileSystem* fileSystem = nullptr;
    uint8_t activeSlot = 0xFF; // 0..7, or 0xFF if none
};

extern MuteBanks muteBanks;

#endif /* MUTE_BANKS_H_ */
