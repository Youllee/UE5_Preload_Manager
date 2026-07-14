#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "YLPreloadManagerType.h"
#include "YLPreloadManagerSettings.generated.h"

UCLASS(Config = YLPreloadManager, DefaultConfig, meta = (DisplayName = "Youllee's Preload Manager"))
class YLPRELOADMANAGER_API UYLPreloadManagerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

	UPROPERTY(Config, EditAnywhere, Category = "YL|Preload")
	TArray<FYLDataAssetPreloadData> DataAssets;

	UPROPERTY(Config, EditAnywhere, Category = "YL|Preload")
	TArray<FYLDataTablePreloadData> DataTables;

	UPROPERTY(Config, EditAnywhere, Category = "YL|Preload")
	TArray<FYLDataRegistryPreloadData> DataRegistries;

};
