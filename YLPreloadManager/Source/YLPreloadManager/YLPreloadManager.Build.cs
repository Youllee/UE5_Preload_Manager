using UnrealBuildTool;

public class YLPreloadManager : ModuleRules
{
	public YLPreloadManager(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "DataRegistry",
				"DeveloperSettings",
				"Engine",
			}
			);
	}
}
