#ifndef PATTERN_LAUNCH_SCENES_H_
#define PATTERN_LAUNCH_SCENES_H_

#include "FeatureModule.h"

#include <stdint.h>

class FileSystem;

static constexpr uint8_t kPatternScenesCount = 8;
static constexpr uint16_t kPatternScenesFileVersion = 1;

struct strPatternScenesFile
{
    char magic[4];
    uint16_t version;
    uint16_t count;
    strPatternScene scenes[kPatternScenesCount];
};

static_assert(sizeof(strPatternScenesFile) % 4 == 0,
              "strPatternScenesFile must be 4-byte aligned for FatFs DMA");

class PatternLaunchScenes : public FeatureModule, public ModulePage
{
  public:
    const char* id() const override { return "patternscenes"; }

    void registerExtensions(ExtensionRegistrar& registrar) override
    {
        registrar.addPage(ScreenId::Performance, this, 20);
    }

    void init(FileSystem* fileSystem) override;
    void onWorkspaceClear() override;
    void onWorkspaceLoad() override;
    void onProjectOpen(const char* projectPath, const char* projectName) override;
    void onProjectSave(const char* projectPath, const char* projectName) override;

    const char* title() const override { return "Patterns"; }
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

extern PatternLaunchScenes patternLaunchScenes;

#endif /* PATTERN_LAUNCH_SCENES_H_ */
