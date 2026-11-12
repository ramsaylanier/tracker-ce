#include "FeatureModuleRegistry.h"

#include "sdk/Registrar.h"

#include "configEditor/configMenuActions.h"

FeatureModuleRegistry featureModules;

void FeatureModuleRegistry::add(FeatureModule* module)
{
    if (module == nullptr || moduleCount >= kMaxModules)
    {
        return;
    }

    modules[moduleCount++] = module;
}

void FeatureModuleRegistry::clear()
{
    moduleCount = 0;
    extensionCatalog().clear();
}

FeatureModule* FeatureModuleRegistry::get(uint8_t index) const
{
    if (index >= moduleCount)
    {
        return nullptr;
    }
    return modules[index];
}

void FeatureModuleRegistry::registerAllExtensions()
{
    extensionCatalog().clear();
    ExtensionRegistrar registrar(extensionCatalog());
    for (uint8_t i = 0; i < moduleCount; i++)
    {
        modules[i]->registerExtensions(registrar);
    }
}

void FeatureModuleRegistry::initAll(FileSystem* fs)
{
    registerAllExtensions();
    // ConfigItems are registered in registerExtensions; populate Config → Modules now.
    rebuildModulesConfigMenu();
    for (uint8_t i = 0; i < moduleCount; i++)
    {
        modules[i]->init(fs);
    }
}

void FeatureModuleRegistry::onWorkspaceClear()
{
    for (uint8_t i = 0; i < moduleCount; i++)
    {
        modules[i]->onWorkspaceClear();
    }
}

void FeatureModuleRegistry::onWorkspaceLoad()
{
    for (uint8_t i = 0; i < moduleCount; i++)
    {
        modules[i]->onWorkspaceLoad();
    }
}

void FeatureModuleRegistry::onProjectOpen(const char* projectPath, const char* projectName)
{
    for (uint8_t i = 0; i < moduleCount; i++)
    {
        modules[i]->onProjectOpen(projectPath, projectName);
    }
}

void FeatureModuleRegistry::onProjectSave(const char* projectPath, const char* projectName)
{
    for (uint8_t i = 0; i < moduleCount; i++)
    {
        modules[i]->onProjectSave(projectPath, projectName);
    }
}

void FeatureModuleRegistry::updateAll()
{
    for (uint8_t i = 0; i < moduleCount; i++)
    {
        modules[i]->update();
    }
}

uint8_t FeatureModuleRegistry::patternStepFxCount() const
{
    return extensionCatalog().patternStepFxCount();
}

const PatternStepFxInfo* FeatureModuleRegistry::patternStepFxAt(uint8_t index) const
{
    return extensionCatalog().patternStepFxAt(index);
}

const PatternStepFxInfo* FeatureModuleRegistry::findPatternStepFx(uint8_t typeId) const
{
    return extensionCatalog().findPatternStepFx(typeId);
}

void FeatureModuleRegistry::dispatchPatternStepFx(uint8_t typeId, uint8_t value)
{
    extensionCatalog().dispatchPatternStepFx(typeId, value);
}
