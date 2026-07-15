// Copyright Epic Games, Inc. All Rights Reserved.

#include "YLPreloadManager.h"
#include "YLPreloadManagerType.h"
#include "YLPreloadManagerSettings.h"

#include "DataRegistrySubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
#include "Templates/UnrealTemplate.h"

#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectGlobals.h"
#endif

#define LOCTEXT_NAMESPACE "FYLPreloadManagerModule"

DEFINE_LOG_CATEGORY_STATIC(LogYLPreloadManager, Log, All);

static const TCHAR* LexToString(EPreloadContext Context)
{
	switch (Context)
	{
	case EPreloadContext::EditorWorld:	return TEXT("EditorWorld");
	case EPreloadContext::GameServer:	return TEXT("GameServer");
	case EPreloadContext::GameClient:	return TEXT("GameClient");
	}

	return TEXT("Unknown");
}

void FYLPreloadManagerModule::StartupModule()
{
	RegisterDelegates();
}

void FYLPreloadManagerModule::ShutdownModule()
{
	UnregisterDelegates();

	ResetPreloadedAssetsForContext(EPreloadContext::EditorWorld);
	ResetPreloadedAssetsForContext(EPreloadContext::GameServer);
	ResetPreloadedAssetsForContext(EPreloadContext::GameClient);
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

	// GameWorld가 초기화되면, NetMode에 맞는 Server / Client 프리로드 에셋을 로드한다.
	PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddLambda(
		[this](UWorld* World, const UWorld::InitializationValues IVS)
		{
			if (World && World->IsGameWorld())
			{
				PreloadAssetsForGameWorld(World);
			}
		});

	// GameWorld의 메모리가 해제되는 시점에, NetMode에 맞는 Server / Client 프리로드 에셋을 언로드한다.
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda(
		[this](UWorld* World, bool bSessionEnded, bool bCleanupResources)
		{
			if (World && World->IsGameWorld())
			{
				ResetPreloadedAssetsForGameWorld(World);
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

	ResetPreloadedAssetsForContext(Context);
	TArray<TSharedPtr<FStreamableHandle>>& TargetAsyncLoadHandles = GetAsyncLoadHandlesForContext(Context);

	auto StoreLoadedObject = [this, Context](UObject* LoadedObject)
		{
			if (LoadedObject == nullptr)
			{
				return;
			}

			GetPreloadedAssetsForContext(Context).Add(TStrongObjectPtr<UObject>(LoadedObject));

#if WITH_EDITOR
			if (Context == EPreloadContext::EditorWorld)
			{
				WeakPreloadAssets.AddUnique(LoadedObject);
			}
#endif
		};

	// Asset을 Load한다. 후처리가 필요한 경우 OnLoaded를 사용한다.
	auto PreloadSoftObject = [this, Context, &TargetAsyncLoadHandles, StoreLoadedObject](const FYLPreloadDataBase& PreloadData, const auto& SoftObjectPtr, auto OnLoaded)
		{
			if (ShouldPreloadForContext(PreloadData, Context) == false)
			{
				return;
			}

			if (SoftObjectPtr.IsNull())
			{
				UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload failed. Asset=None, LoadType=%s, Context=%s, Reason=SoftObjectPtr is null"),
					PreloadData.bUseAsyncLoad ? TEXT("Async") : TEXT("Sync"),
					LexToString(Context));
				return;
			}

			// 이미 에셋이 로드되어 있으면 Skip한다.
			const FSoftObjectPath ObjectPath = SoftObjectPtr.ToSoftObjectPath();
			if (UObject* LoadedObject = ObjectPath.ResolveObject())
			{
				StoreLoadedObject(LoadedObject);

				UE_LOG(LogYLPreloadManager, Log, TEXT("Preload skipped. Asset=%s, LoadType=%s, Context=%s, Reason=Asset is already loaded"),
					*ObjectPath.ToString(),
					PreloadData.bUseAsyncLoad ? TEXT("Async") : TEXT("Sync"),
					LexToString(Context));
				return;
			}

			if (PreloadData.bUseAsyncLoad)
			{
				// Async Load
				TSharedPtr<FStreamableHandle> AsyncLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
					ObjectPath,
					FStreamableDelegate::CreateLambda(
						[Context, ObjectPath, StoreLoadedObject, OnLoaded]() mutable
						{
							if (UObject* LoadedObject = ObjectPath.ResolveObject())
							{
								StoreLoadedObject(LoadedObject);
								OnLoaded(LoadedObject);

								UE_LOG(LogYLPreloadManager, Log, TEXT("Preload completed. Asset=%s, LoadType=Async, Context=%s"),
									*ObjectPath.ToString(),
									LexToString(Context));
							}
							else
							{
								UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload failed. Asset=%s, LoadType=Async, Context=%s, Reason=Loaded object could not be resolved after async request completed"),
									*ObjectPath.ToString(),
									LexToString(Context));
							}
						}));

				if (AsyncLoadHandle.IsValid())
				{
					TargetAsyncLoadHandles.Add(AsyncLoadHandle);
				}
				else
				{
					UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload failed. Asset=%s, LoadType=Async, Context=%s, Reason=RequestAsyncLoad returned an invalid handle"),
						*ObjectPath.ToString(),
						LexToString(Context));
				}
			}
			else
			{
				// Sync Load
				if (UObject* LoadedObject = SoftObjectPtr.LoadSynchronous())
				{
					StoreLoadedObject(LoadedObject);
					OnLoaded(LoadedObject);

					UE_LOG(LogYLPreloadManager, Log, TEXT("Preload completed. Asset=%s, LoadType=Sync, Context=%s"),
						*ObjectPath.ToString(),
						LexToString(Context));
				}
				else
				{
					UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload failed. Asset=%s, LoadType=Sync, Context=%s, Reason=LoadSynchronous returned null"),
						*ObjectPath.ToString(),
						LexToString(Context));
				}
			}
		};

	// DataAsset Preload
	for (const FYLDataAssetPreloadData& DataAssetData : Settings->DataAssets)
	{
		PreloadSoftObject(DataAssetData, DataAssetData.DataAsset, [](UObject*) {});
	}

	// DataTable Preload
	for (const FYLDataTablePreloadData& DataTableData : Settings->DataTables)
	{
		PreloadSoftObject(DataTableData, DataTableData.DataTable, [](UObject*) {});
	}

	// DataRegistry Preload
	UDataRegistrySubsystem* DataRegistrySubsystem = UDataRegistrySubsystem::Get();
	if (DataRegistrySubsystem)
	{
		for (const FYLDataRegistryPreloadData& DataRegistryData : Settings->DataRegistries)
		{
			const FSoftObjectPath DataRegistryPath = DataRegistryData.DataRegistry.ToSoftObjectPath();
			PreloadSoftObject(
				DataRegistryData,
				DataRegistryData.DataRegistry,
				[DataRegistryPath](UObject* LoadedObject)
				{
					if (UDataRegistrySubsystem* LoadedDataRegistrySubsystem = UDataRegistrySubsystem::Get())
					{
						LoadedDataRegistrySubsystem->LoadRegistryPath(DataRegistryPath);
					}
					else
					{
						UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload post-process failed. Asset=%s, Reason=DataRegistrySubsystem is not available"),
							*DataRegistryPath.ToString());
					}

					if (UDataRegistry* DataRegistry = Cast<UDataRegistry>(LoadedObject))
					{
						if (DataRegistry->IsInitialized())
						{
							DataRegistry->ResetRuntimeState();
						}
						else
						{
							UE_LOG(LogYLPreloadManager, Log, TEXT("Preload post-process skipped. Asset=%s, Reason=DataRegistry is not initialized"),
								*DataRegistryPath.ToString());
						}
					}
					else
					{
						UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload post-process failed. Asset=%s, Reason=Loaded object is not a DataRegistry"),
							*DataRegistryPath.ToString());
					}
				});
		}
	}
}

void FYLPreloadManagerModule::PreloadAssetsForGameWorld(const UWorld* World)
{
	if (World == nullptr)
	{
		return;
	}

	switch (World->GetNetMode())
	{
	case NM_DedicatedServer:
		PreloadAssets(EPreloadContext::GameServer);
		break;
	case NM_Client:
		PreloadAssets(EPreloadContext::GameClient);
		break;
	case NM_ListenServer:
	case NM_Standalone:
		PreloadAssets(EPreloadContext::GameServer);
		PreloadAssets(EPreloadContext::GameClient);
		break;
	default:
		break;
	}
}

void FYLPreloadManagerModule::ResetPreloadedAssetsForGameWorld(const UWorld* World)
{
	if (World == nullptr)
	{
		return;
	}

	switch (World->GetNetMode())
	{
	case NM_DedicatedServer:
		ResetPreloadedAssetsForContext(EPreloadContext::GameServer);
		break;
	case NM_Client:
		ResetPreloadedAssetsForContext(EPreloadContext::GameClient);
		break;
	case NM_ListenServer:
	case NM_Standalone:
		ResetPreloadedAssetsForContext(EPreloadContext::GameServer);
		ResetPreloadedAssetsForContext(EPreloadContext::GameClient);
		break;
	default:
		break;
	}
}

TArray<TStrongObjectPtr<UObject>>& FYLPreloadManagerModule::GetPreloadedAssetsForContext(EPreloadContext Context)
{
	switch (Context)
	{
	case EPreloadContext::EditorWorld:	return EditorWorldPreloadedAssets;
	case EPreloadContext::GameServer:	return GameServerPreloadedAssets;
	case EPreloadContext::GameClient:	return GameClientPreloadedAssets;
	}

	return EditorWorldPreloadedAssets;
}

TArray<TSharedPtr<FStreamableHandle>>& FYLPreloadManagerModule::GetAsyncLoadHandlesForContext(EPreloadContext Context)
{
	switch (Context)
	{
	case EPreloadContext::EditorWorld:	return EditorWorldAsyncLoadHandles;
	case EPreloadContext::GameServer:	return GameServerAsyncLoadHandles;
	case EPreloadContext::GameClient:	return GameClientAsyncLoadHandles;
	}

	return EditorWorldAsyncLoadHandles;
}

void FYLPreloadManagerModule::ResetPreloadedAssetsForContext(EPreloadContext Context)
{
	GetAsyncLoadHandlesForContext(Context).Reset();
	GetPreloadedAssetsForContext(Context).Reset();
}

bool FYLPreloadManagerModule::ShouldPreloadForContext(const FYLPreloadDataBase& InPreloadData, EPreloadContext Context) const
{
	switch (Context)
	{
	case EPreloadContext::EditorWorld:	return InPreloadData.bPreloadOnEditor;
	case EPreloadContext::GameServer:	return InPreloadData.bPreloadOnServer;
	case EPreloadContext::GameClient:	return InPreloadData.bPreloadOnClient;
	}

	return false;
}

#if WITH_EDITOR
void FYLPreloadManagerModule::OnRefreshPreloadAssets()
{
	if (bIsRefreshingPreload == true)
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
