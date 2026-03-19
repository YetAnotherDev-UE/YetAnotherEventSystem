// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class YetAnotherEventEditorTarget : TargetRules
{
	public YetAnotherEventEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "YetAnotherEvent" } );

        // Run our python script as a prebuild step
        PreBuildSteps.Add("python \"$(ProjectDir)\\UEventGenerator.py\"");
    }
}
