#pragma once

#include "CoreMinimal.h"
#include "DataRegistry.h"
#include "Engine/DataTable.h"
#include "YLPreloadManagerType.generated.h"

UENUM()
enum class EPreloadContext : uint8
{
	EditorWorld,
	GameWorld,
};

USTRUCT()
struct YLPRELOADMANAGER_API FYLPreloadDataBase
{
	GENERATED_BODY()

public:
	// Asset을 Editor World에서 Preload 합니다.
	UPROPERTY(EditAnywhere, Category = "YL|Preload", meta = (DisplayPriority = 2))
	bool bPreloadOnEditor = false;
	// Asset을 Game World에서 Preload 합니다.
	UPROPERTY(EditAnywhere, Category = "YL|Preload", meta = (DisplayPriority = 2))
	bool bPreloadOnGame = false;
};

USTRUCT(BlueprintType, meta = (PresentAsType = "YLPreloadDataBase"))
struct YLPRELOADMANAGER_API FYLDataAssetPreloadData : public FYLPreloadDataBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "YL|Preload", meta = (DisplayPriority = 1, AllowedClasses = "/Script/Engine.DataAsset"))
	TSoftObjectPtr<UObject> DataAsset;
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
