// Fill out your copyright notice in the Description page of Project Settings.

using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using UnrealBuildTool;

public class YetAnotherEventTarget : TargetRules
{
	public YetAnotherEventTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "YetAnotherEvent" } );

        // Determine host OS
        bool bWindows = RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
        string PythonCommand = bWindows ? "python" : "python3";

        // Use forward slashes: UBT will automatically normalize them for the specific OS (POSIX standard)
        PreBuildSteps.Add($"{PythonCommand} \"$(ProjectDir)/UEventGenerator.py\"");
    }
}
