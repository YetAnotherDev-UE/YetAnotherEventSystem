// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;
using System.Diagnostics;

public class YetAnotherEvent : ModuleRules
{
	public YetAnotherEvent(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		//// Find absolute path
		//string projectDirectory = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
		//string pythonScriptPath = Path.Combine(projectDirectory, "UEventGenerator.py");

		//// Background process
		//ProcessStartInfo startInfo = new ProcessStartInfo();
		//startInfo.FileName = "python"; // "python3" if on macOS/Linux
		//startInfo.Arguments = $"\"{pythonScriptPath}\"";
		//startInfo.UseShellExecute = false;
		//startInfo.RedirectStandardOutput = true;
		//startInfo.CreateNoWindow = true;

		//// Execute the script and force compiler to wait
		//using (Process process = Process.Start(startInfo)) {
		//	process.WaitForExit();

		//	// Print python output to IDE output log
		//	string output = process.StandardOutput.ReadToEnd();
		//	if (!string.IsNullOrEmpty(output)) {
		//		System.Console.WriteLine("UEVENT GENERATOR: " + output);
		//	}
		//}


		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
