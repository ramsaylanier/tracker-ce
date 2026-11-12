#ifndef MODULES_SDK_FEATUREMODULE_H_
#define MODULES_SDK_FEATUREMODULE_H_

#include "Registrar.h"

class FileSystem;

// Compile-time feature module linked into the firmware image.
// External modules live in FEATURE_MODULE_DIRS and are registered from the
// generated registry (see docs/feature_module_sdk.md).
class FeatureModule
{
  public:
    virtual ~FeatureModule() {}

    // Sidecar basename without extension, e.g. "perfscenes" -> perfscenes.bin
    virtual const char* id() const = 0;

    // Called once after registerFeatureModules, before init.
    // Register pages, columns, softkeys, Step FX, config items, listeners.
    virtual void registerExtensions(ExtensionRegistrar& registrar) { (void)registrar; }

    virtual void init(FileSystem* fs) { (void)fs; }
    virtual void onWorkspaceClear() {}
    virtual void onWorkspaceLoad() {}
    virtual void onProjectOpen(const char* projectPath, const char* projectName)
    {
        (void)projectPath;
        (void)projectName;
    }
    virtual void onProjectSave(const char* projectPath, const char* projectName)
    {
        (void)projectPath;
        (void)projectName;
    }

    // Main thread only. Do not call from the audio callback.
    virtual void update() {}
};

#endif /* MODULES_SDK_FEATUREMODULE_H_ */
