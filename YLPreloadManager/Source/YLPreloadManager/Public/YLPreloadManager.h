#pragma once

#include "Modules/ModuleManager.h"
#include "UObject/StrongObjectPtr.h"

class UObject;
class UPackage;
class UWorld;
class UYLPreloadManagerSettings;
struct FStreamableHandle;
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
	void PreloadAssetsForGameWorld(const UWorld* World);
	void ResetPreloadedAssetsForGameWorld(const UWorld* World);

	TArray<TStrongObjectPtr<UObject>>& GetPreloadedAssetsForContext(EPreloadContext Context);
	TArray<TSharedPtr<FStreamableHandle>>& GetAsyncLoadHandlesForContext(EPreloadContext Context);
	void ResetPreloadedAssetsForContext(EPreloadContext Context);
	bool ShouldPreloadForContext(const FYLPreloadDataBase& InPreloadData, EPreloadContext Context) const;

private:
	TArray<TStrongObjectPtr<UObject>> EditorWorldPreloadedAssets;
	TArray<TStrongObjectPtr<UObject>> GameServerPreloadedAssets;
	TArray<TStrongObjectPtr<UObject>> GameClientPreloadedAssets;

	TArray<TSharedPtr<FStreamableHandle>> EditorWorldAsyncLoadHandles;
	TArray<TSharedPtr<FStreamableHandle>> GameServerAsyncLoadHandles;
	TArray<TSharedPtr<FStreamableHandle>> GameClientAsyncLoadHandles;

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
