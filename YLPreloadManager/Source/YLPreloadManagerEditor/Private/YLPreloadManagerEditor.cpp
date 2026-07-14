#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "YLPreloadManagerType.h"
#include "YLPreloadManagerCustomization.h"

class FYLPreloadManagerEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

			PropertyEditorModule.RegisterCustomPropertyTypeLayout(FYLPreloadDataBase::StaticStruct()->GetFName()
				, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FYLPreloadDataCustomization::MakeInstance));

			PropertyEditorModule.NotifyCustomizationModuleChanged();
		}
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

			PropertyEditorModule.UnregisterCustomPropertyTypeLayout(FYLPreloadDataBase::StaticStruct()->GetFName());

			PropertyEditorModule.NotifyCustomizationModuleChanged();
		}
	}
};

IMPLEMENT_MODULE(FYLPreloadManagerEditorModule, YLPreloadManagerEditor)
