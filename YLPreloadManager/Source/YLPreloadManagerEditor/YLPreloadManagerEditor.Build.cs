using UnrealBuildTool;

public class YLPreloadManagerEditor : ModuleRules
{
	public YLPreloadManagerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"YLPreloadManager",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"PropertyEditor",
				"Slate",
				"SlateCore",
			}
		);
	}
}
