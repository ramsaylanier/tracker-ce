#ifndef MODULES_FEATUREMODULEREGISTRY_H_
#define MODULES_FEATUREMODULEREGISTRY_H_

#include "sdk/FeatureModule.h"
#include "sdk/Registrar.h"

#include <stdint.h>

class FileSystem;

class FeatureModuleRegistry
{
  public:
    static constexpr uint8_t kMaxModules = 8;

    void add(FeatureModule* module);
    void clear();

    void initAll(FileSystem* fs);
    void onWorkspaceClear();
    void onWorkspaceLoad();
    void onProjectOpen(const char* projectPath, const char* projectName);
    void onProjectSave(const char* projectPath, const char* projectName);
    void updateAll();

    uint8_t count() const { return moduleCount; }
    FeatureModule* get(uint8_t index) const;

    // Pattern Step FX lookups used by the sequencer and the FX picker.
    uint8_t patternStepFxCount() const;
    const PatternStepFxInfo* patternStepFxAt(uint8_t index) const;
    const PatternStepFxInfo* findPatternStepFx(uint8_t typeId) const;
    void dispatchPatternStepFx(uint8_t typeId, uint8_t value);

  private:
    void registerAllExtensions();

    FeatureModule* modules[kMaxModules] = {};
    uint8_t moduleCount = 0;
};

void registerFeatureModules(FeatureModuleRegistry& registry);

extern FeatureModuleRegistry featureModules;

#endif /* MODULES_FEATUREMODULEREGISTRY_H_ */
