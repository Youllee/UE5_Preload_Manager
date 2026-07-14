#pragma once

#include "Modules/ModuleManager.h"
#include "UObject/StrongObjectPtr.h"

class UObject;
class UPackage;
class UWorld;
class UYLPreloadManagerSettings;
class FObjectPostSaveContext;
struct FPropertyChangedEvent;
struct FYLPreloadDataBase;
enum class EPreloadContext : uint8;

class FYLPreloadManagerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterDelegates();
	void UnregisterDelegates();
	void PreloadAssets(EPreloadContext Context);
	bool ShouldPreloadForContext(const FYLPreloadDataBase& InPreloadData, EPreloadContext Context) const;

private:
	TArray<TStrongObjectPtr<UObject>> EditorWorldPreloadedAssets;
	TArray<TStrongObjectPtr<UObject>> GameWorldPreloadedAssets;

	FDelegateHandle PostEngineInitHandle;
	FDelegateHandle PostWorldInitializationHandle;
	FDelegateHandle WorldCleanupHandle;

#if WITH_EDITOR
private:
	void OnRefreshPreloadAssets();
	void OnPreloadAssetSaved(const FString& InPackageFileName, UPackage* InPackage, FObjectPostSaveContext InObjectSaveContext);

private:
	bool bIsRefreshingPreload = false;
	TArray<TWeakObjectPtr<UObject>> WeakPreloadAssets;
	TWeakObjectPtr<UYLPreloadManagerSettings> WeakPreloadSettings;

	FDelegateHandle PreloadSettingsChangedHandle;
	FDelegateHandle PreloadAssetSavedHandle;
#endif

};
