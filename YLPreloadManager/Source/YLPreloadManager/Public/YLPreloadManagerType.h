#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "DataRegistry.h"
#include "YLPreloadManagerType.generated.h"

UENUM()
enum class EPreloadContext : uint8
{
	EditorWorld,
	GameServer,
	GameClient,
};

USTRUCT()
struct YLPRELOADMANAGER_API FYLPreloadDataBase
{
	GENERATED_BODY()

public:
	// Asset을 Editor World에서 Preload 합니다.
	UPROPERTY(EditAnywhere, Category = "YL|Preload", meta = (DisplayPriority = 2))
	bool bPreloadOnEditor = false;
	// Asset을 Game Server에서 Preload 합니다.
	UPROPERTY(EditAnywhere, Category = "YL|Preload", meta = (DisplayPriority = 2))
	bool bPreloadOnServer = false;
	// Asset을 Game Client에서 Preload 합니다.
	UPROPERTY(EditAnywhere, Category = "YL|Preload", meta = (DisplayPriority = 2))
	bool bPreloadOnClient = false;
	// Asset을 Async Load 합니다.
	UPROPERTY(EditAnywhere, Category = "YL|Preload", meta = (DisplayPriority = 2))
	bool bUseAsyncLoad = false;
};

USTRUCT(BlueprintType, meta = (PresentAsType = "YLPreloadDataBase"))
struct YLPRELOADMANAGER_API FYLDataAssetPreloadData : public FYLPreloadDataBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "YL|Preload", meta = (DisplayPriority = 1))
	TSoftObjectPtr<UDataAsset> DataAsset;
};

USTRUCT(BlueprintType, meta = (PresentAsType = "YLPreloadDataBase"))
struct YLPRELOADMANAGER_API FYLDataTablePreloadData : public FYLPreloadDataBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "YL|Preload", meta = (DisplayPriority = 1))
	TSoftObjectPtr<UDataTable> DataTable;
};

USTRUCT(BlueprintType, meta = (PresentAsType = "YLPreloadDataBase"))
struct YLPRELOADMANAGER_API FYLDataRegistryPreloadData : public FYLPreloadDataBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "YL|Preload", meta = (DisplayPriority = 1))
	TSoftObjectPtr<UDataRegistry> DataRegistry;
};
