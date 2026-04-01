// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class YetAnotherEventTarget : TargetRules
{
	public YetAnotherEventTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "YetAnotherEvent" } );

        // Run our python script as a prebuild step
        // PreBuildSteps.Add("python \"$(ProjectDir)\\UEventGenerator.py\"");
    }
}
