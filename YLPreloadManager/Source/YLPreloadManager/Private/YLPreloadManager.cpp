// Copyright Epic Games, Inc. All Rights Reserved.

#include "YLPreloadManager.h"
#include "YLPreloadManagerType.h"
#include "YLPreloadManagerSettings.h"

#include "DataRegistrySubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
#include "Templates/UnrealTemplate.h"

#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectGlobals.h"
#endif

#define LOCTEXT_NAMESPACE "FYLPreloadManagerModule"

DEFINE_LOG_CATEGORY_STATIC(LogYLPreloadManager, Log, All);

void FYLPreloadManagerModule::StartupModule()
{
	RegisterDelegates();
}

void FYLPreloadManagerModule::ShutdownModule()
{
	UnregisterDelegates();

	EditorWorldPreloadedAssets.Reset();
	GameWorldPreloadedAssets.Reset();
}

void FYLPreloadManagerModule::RegisterDelegates()
{
	// 엔진 초기화 완료 직후, bPreloadOnEditor가 true인 에셋을 로드한다.
	PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddLambda(
		[this]()
		{
			{
				PreloadAssets(EPreloadContext::EditorWorld);
			}
		});

	// GameWorld가 초기화되면, bPreloadOnGame가 true인 에셋을 로드한다.
	PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddLambda(
		[this](UWorld* World, const UWorld::InitializationValues IVS)
		{
			if (World && World->IsGameWorld())
			{
				PreloadAssets(EPreloadContext::GameWorld);
			}
		});

	// GameWorld의 메모리가 해제되는 시점에, bPreloadOnGame가 true인 에셋을 언로드한다.
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda(
		[this](UWorld* World, bool bSessionEnded, bool bCleanupResources)
		{
			if (World && World->IsGameWorld())
			{
				GameWorldPreloadedAssets.Reset();
			}
		});

#if WITH_EDITOR
	// Editor의 Preload Manager Setting이 수정된 경우, 다시 프리로드한다.
	if (UYLPreloadManagerSettings* PreloadSettings = GetMutableDefault<UYLPreloadManagerSettings>())
	{
		WeakPreloadSettings = PreloadSettings;
		PreloadSettingsChangedHandle = PreloadSettings->OnSettingChanged().AddLambda(
			[this](UObject* SettingsObject, FPropertyChangedEvent& PropertyChangedEvent)
			{
				OnRefreshPreloadAssets();
			});
	}
	else
	{
		WeakPreloadSettings.Reset();
	}

	// 패키지가 저장된 경우, 패키지에 감시 대상이 포함되어 있으면, 다시 프리로드한다.
	PreloadAssetSavedHandle = UPackage::PackageSavedWithContextEvent.AddLambda(
		[this](const FString& InPackageFileName, UPackage* InPackage, FObjectPostSaveContext InObjectSaveContext)
		{
			OnPreloadAssetSaved(InPackageFileName, InPackage, InObjectSaveContext);
		});
#endif
}

void FYLPreloadManagerModule::UnregisterDelegates()
{
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}

	if (PostWorldInitializationHandle.IsValid())
	{
		FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle);
		PostWorldInitializationHandle.Reset();
	}

	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}

#if WITH_EDITOR
	if (PreloadSettingsChangedHandle.IsValid())
	{
		if (UYLPreloadManagerSettings* PreloadSettings = WeakPreloadSettings.Get())
		{
			PreloadSettings->OnSettingChanged().Remove(PreloadSettingsChangedHandle);
		}

		PreloadSettingsChangedHandle.Reset();
		WeakPreloadSettings.Reset();
	}

	if (PreloadAssetSavedHandle.IsValid())
	{
		UPackage::PackageSavedWithContextEvent.Remove(PreloadAssetSavedHandle);
		PreloadAssetSavedHandle.Reset();
	}
#endif
}

void FYLPreloadManagerModule::PreloadAssets(EPreloadContext Context)
{
	const UYLPreloadManagerSettings* Settings = GetDefault<UYLPreloadManagerSettings>();
	if (Settings == nullptr)
	{
		return;
	}

#if WITH_EDITOR
	if (Context == EPreloadContext::EditorWorld)
	{
		WeakPreloadAssets.Reset();
	}
#endif

	TArray<TStrongObjectPtr<UObject>>& TargetPreloadedAssets = (Context == EPreloadContext::EditorWorld) ? EditorWorldPreloadedAssets : GameWorldPreloadedAssets;
	TargetPreloadedAssets.Reset();

	// DataAsset Preload
	for (const FYLDataAssetPreloadData& DataAssetData : Settings->DataAssets)
	{
		if (ShouldPreloadForContext(DataAssetData, Context) == false)
		{
			continue;
		}

		const TSoftObjectPtr<UDataAsset>& AssetPtr = DataAssetData.DataAsset;
		if (AssetPtr.IsNull())
		{
			continue;
		}

		if (UDataAsset* Asset = AssetPtr.LoadSynchronous())
		{
			TargetPreloadedAssets.Add(TStrongObjectPtr<UDataAsset>(Asset));

#if WITH_EDITOR
			if (Context == EPreloadContext::EditorWorld)
			{
				WeakPreloadAssets.AddUnique(Asset);
			}
#endif
		}
	}

	// DataTable Preload
	for (const FYLDataTablePreloadData& DataTableData : Settings->DataTables)
	{
		if (ShouldPreloadForContext(DataTableData, Context) == false)
		{
			continue;
		}

		const TSoftObjectPtr<UDataTable>& DataTablePtr = DataTableData.DataTable;
		if (DataTablePtr.IsNull())
		{
			continue;
		}

		if (UDataTable* DataTable = DataTablePtr.LoadSynchronous())
		{
			TargetPreloadedAssets.Add(TStrongObjectPtr<UObject>(DataTable));

#if WITH_EDITOR
			if (Context == EPreloadContext::EditorWorld)
			{
				WeakPreloadAssets.AddUnique(DataTable);
			}
#endif
		}
	}

	// DataRegistry Preload
	UDataRegistrySubsystem* DataRegistrySubsystem = UDataRegistrySubsystem::Get();
	if (DataRegistrySubsystem)
	{
		for (const FYLDataRegistryPreloadData& DataRegistryData : Settings->DataRegistries)
		{
			if (ShouldPreloadForContext(DataRegistryData, Context) == false)
			{
				continue;
			}

			const TSoftObjectPtr<UDataRegistry>& DataRegistryPtr = DataRegistryData.DataRegistry;
			if (DataRegistryPtr.IsNull())
			{
				continue;
			}

			DataRegistrySubsystem->LoadRegistryPath(DataRegistryPtr.ToSoftObjectPath());
			if (UDataRegistry* DataRegistry = DataRegistryPtr.LoadSynchronous())
			{
				TargetPreloadedAssets.Add(TStrongObjectPtr<UObject>(DataRegistry));

#if WITH_EDITOR
				if (Context == EPreloadContext::EditorWorld)
				{
					WeakPreloadAssets.AddUnique(DataRegistry);
				}
#endif

				if (DataRegistry->IsInitialized())
				{
					DataRegistry->ResetRuntimeState();
				}
			}
		}
	}
}

bool FYLPreloadManagerModule::ShouldPreloadForContext(const FYLPreloadDataBase& InPreloadData, EPreloadContext Context) const
{
	switch (Context)
	{
	case EPreloadContext::EditorWorld:	return InPreloadData.bPreloadOnEditor;
	case EPreloadContext::GameWorld:	return InPreloadData.bPreloadOnGame;
	}

	return false;
}

#if WITH_EDITOR
void FYLPreloadManagerModule::OnRefreshPreloadAssets()
{
	if (bIsRefreshingPreload)
	{
		return;
	}
	TGuardValue<bool> RefreshGuard(bIsRefreshingPreload, true);

	PreloadAssets(EPreloadContext::EditorWorld);
}

void FYLPreloadManagerModule::OnPreloadAssetSaved(const FString& /*InPackageFileName*/, UPackage* InPackage, FObjectPostSaveContext InObjectSaveContext)
{
	if (InObjectSaveContext.IsCooking() || InPackage == nullptr)
	{
		return;
	}

	TArray<UObject*> SavedObjects;
	constexpr bool bIncludeNestedObjects = false;
	GetObjectsWithPackage(InPackage, SavedObjects, bIncludeNestedObjects);
	for (UObject* SavedObject : SavedObjects)
	{
		if (SavedObject == nullptr)
		{
			continue;
		}

		// 감시 대상이 패키지에 포함되어 있으면, 다시 로드한다.
		for (const TWeakObjectPtr<UObject>& WatchedAsset : WeakPreloadAssets)
		{
			if (WatchedAsset.Get() == SavedObject)
			{
				OnRefreshPreloadAssets();
				return;
			}
		}
	}
}
#endif

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FYLPreloadManagerModule, YLPreloadManager)
