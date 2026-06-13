using UnrealBuildTool;

public class YetAnotherEvent : ModuleRules
{
    public YetAnotherEvent(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "YetAnotherEventRuntime"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
