using System;
using System.IO;
using UnrealBuildTool;
using System.Text.RegularExpressions;
using System.Collections.Generic;

public class YetAnotherEventRuntime : ModuleRules
{
    public YetAnotherEventRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });

        // Scan all project/plugin headers for UEVENT declarations

        if (Target.ProjectFile == null) return;

        string ProjectRoot = Target.ProjectFile.Directory.FullName;
        string PluginRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", ".."));
        string[] SourceRoots = { Path.Combine(ProjectRoot, "Source"), Path.Combine(PluginRoot, "Source") };

        HashSet<string> UniqueHeaderPaths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        foreach (string SourceRoot in SourceRoots)
        {
            if (!Directory.Exists(SourceRoot)) continue;

            foreach (string HeaderPath in Directory.EnumerateFiles(SourceRoot, "*.h", SearchOption.AllDirectories))
            {
                UniqueHeaderPaths.Add(Path.GetFullPath(HeaderPath));
            }
        }

        List<string> HeaderPaths = new List<string>(UniqueHeaderPaths);
        HeaderPaths.Sort(StringComparer.OrdinalIgnoreCase);

        const string EventPattern =
            @"UEVENT\s*\((?<Metadata>.*?)\)\s*" +
            @"TEvent\s*<(?<Parameters>.*?)>\s*" +
            @"(?<EventName>[A-Za-z_][A-Za-z0-9_]*)\s*" +
            @"(?:\{\})?\s*;";

        int DetectedEventCount = 0;

        foreach (string HeaderPath in HeaderPaths)
        {
            ExternalDependencies.Add(HeaderPath);

            string HeaderText = File.ReadAllText(HeaderPath);

            MatchCollection EventMatches = Regex.Matches(HeaderText, EventPattern, RegexOptions.Singleline);

            string RelativeHeaderPath = Path.GetRelativePath(ProjectRoot, HeaderPath);
            string NewLine = HeaderText.Contains("\r\n") ? "\r\n" : "\n";

            if (EventMatches.Count == 0)
            {
                const string ExistingGeneratedBeginMarker = "// YAE_GENERATED_CODE_BEGIN";
                const string ExistingGeneratedEndMarker = "// YAE_GENERATED_CODE_END";

                int ExistingGeneratedBeginIndex = HeaderText.IndexOf(ExistingGeneratedBeginMarker, StringComparison.Ordinal);

                int ExistingGeneratedEndIndex = HeaderText.IndexOf(ExistingGeneratedEndMarker, StringComparison.Ordinal);

                // No UEVENTs and no generated region: nothing to clean up.
                if (ExistingGeneratedBeginIndex < 0 && ExistingGeneratedEndIndex < 0) continue;

                if ((ExistingGeneratedBeginIndex < 0) != (ExistingGeneratedEndIndex < 0))
                {
                    throw new BuildException(
                        $"[YetAnotherEvent] Header '{RelativeHeaderPath}' contains only " +
                        "one YAE generated-code marker. Both markers must exist."
                    );
                }

                if (ExistingGeneratedEndIndex < ExistingGeneratedBeginIndex)
                {
                    throw new BuildException(
                        $"[YetAnotherEvent] Header '{RelativeHeaderPath}' has its YAE " +
                        "generated-code end marker before its begin marker."
                    );
                }

                int ExistingGeneratedBeginLineStart = HeaderText.LastIndexOf('\n', ExistingGeneratedBeginIndex);
                ExistingGeneratedBeginLineStart = ExistingGeneratedBeginLineStart < 0
                    ? 0
                    : ExistingGeneratedBeginLineStart + 1;

                int ExistingGeneratedEndLineEnd = HeaderText.IndexOf('\n', ExistingGeneratedEndIndex + ExistingGeneratedEndMarker.Length);
                ExistingGeneratedEndLineEnd = ExistingGeneratedEndLineEnd < 0
                    ? HeaderText.Length
                    : ExistingGeneratedEndLineEnd + 1;

                string CleanedHeaderText = HeaderText.Substring(0, ExistingGeneratedBeginLineStart) + HeaderText.Substring(ExistingGeneratedEndLineEnd);

                if (!string.Equals(HeaderText, CleanedHeaderText, StringComparison.Ordinal))
                {
                    File.WriteAllText(HeaderPath, CleanedHeaderText);

                    Console.WriteLine($"[YetAnotherEvent] Removed stale generated UEVENT region from: " + HeaderPath);
                }

                continue;
            }

            List<string> GeneratedEventSections = new List<string>();

            string HeaderOwningClassName = string.Empty;

            foreach (Match EventMatch in EventMatches)
            {
                string EventName = EventMatch.Groups["EventName"].Value.Trim();

                string OwningClassName = FindOwningReflectedClass(HeaderText, EventMatch.Index);

                if (string.IsNullOrWhiteSpace(OwningClassName))
                {
                    throw new BuildException(
                        $"[YetAnotherEvent] Could not determine the owning reflected class " +
                        $"for UEVENT '{EventName}' in '{RelativeHeaderPath}'."
                    );
                }

                if (string.IsNullOrEmpty(HeaderOwningClassName))
                {
                    HeaderOwningClassName = OwningClassName;
                }
                else if (!string.Equals(HeaderOwningClassName, OwningClassName, StringComparison.Ordinal))
                {
                    throw new BuildException(
                        $"[YetAnotherEvent] Header '{RelativeHeaderPath}' contains UEVENT " +
                        $"declarations owned by multiple classes. Current owners: " +
                        $"'{HeaderOwningClassName}' and '{OwningClassName}'."
                    );
                }


                string EventParameters = EventMatch.Groups["Parameters"].Value.Trim();
                string EventMetadata = EventMatch.Groups["Metadata"].Value.Trim();
                List<string> ParameterTypes = SplitTopLevelCommaSeparated(EventParameters);
                List<string> ParameterNames = new List<string>();

                Match ParamNamesMatch = Regex.Match(EventMetadata, @"ParamNames\s*=\s*\((?<Names>.*?)\)", RegexOptions.Singleline);

                if (ParamNamesMatch.Success)
                {
                    MatchCollection NameMatches = Regex.Matches(ParamNamesMatch.Groups["Names"].Value, @"""(?<Name>[^""]+)""");

                    foreach (Match NameMatch in NameMatches)
                    {
                        ParameterNames.Add(NameMatch.Groups["Name"].Value.Trim());
                    }
                }

                RejectUnsupportedNetworkMetadata(RelativeHeaderPath, OwningClassName, EventName, EventMetadata);

                ValidateExplicitParameterNames(RelativeHeaderPath, OwningClassName, EventName,
                    ParameterNames, ParameterTypes.Count, HasMetadataFlag(EventMetadata, "BlueprintCallable"));

                EmitParameterTypeWarnings(RelativeHeaderPath, OwningClassName, EventName, ParameterTypes);

                ++DetectedEventCount;

                Console.WriteLine($"[YetAnotherEvent] C# detected UEVENT: " +
                    $"{OwningClassName}::{EventName} in {RelativeHeaderPath}"
                );

                Console.WriteLine($"[YetAnotherEvent] Parameter count: " +
                    $"{ParameterTypes.Count}, ParamNames count: " +
                    $"{ParameterNames.Count}"
                );

                if (ParameterNames.Count > ParameterTypes.Count)
                {
                    int IgnoredNameCount = ParameterNames.Count - ParameterTypes.Count;

                    Console.WriteLine($"[YetAnotherEvent][Warning] UEVENT " +
                        $"'{OwningClassName}::{EventName}' provides " +
                        $"{IgnoredNameCount} extra ParamNames. " +
                        "Extra names will be ignored."
                    );
                }
                else if (ParameterNames.Count < ParameterTypes.Count)
                {
                    int GeneratedNameCount = ParameterTypes.Count - ParameterNames.Count;

                    Console.WriteLine($"[YetAnotherEvent][Warning] UEVENT " +
                        $"'{OwningClassName}::{EventName}' is missing " +
                        $"{GeneratedNameCount} ParamNames. " +
                        "Fallback names will be generated."
                    );
                }

                List<string> EffectiveParameterNames = new List<string>();

                for (int Index = 0; Index < ParameterTypes.Count; ++Index)
                {
                    string ParameterName = Index < ParameterNames.Count && !string.IsNullOrWhiteSpace(ParameterNames[Index])
                            ? ParameterNames[Index]
                            : $"Param{Index + 1}";

                    EffectiveParameterNames.Add(ParameterName);
                }

                string DelegateMacroName = GetDelegateMacroName(ParameterTypes.Count);

                List<string> DelegateArguments = new List<string>();
                List<string> CallableParameters = new List<string>();

                for (int Index = 0; Index < ParameterTypes.Count; ++Index)
                {
                    DelegateArguments.Add(ParameterTypes[Index]);
                    DelegateArguments.Add(EffectiveParameterNames[Index]);
                    CallableParameters.Add(ParameterTypes[Index] + " " + EffectiveParameterNames[Index]);
                }

                string DelegateArgumentsText = DelegateArguments.Count > 0
                        ? ", " + string.Join(", ", DelegateArguments)
                        : string.Empty;

                string CallableParametersText = string.Join(", ", CallableParameters);
                string CallableArgumentsText = string.Join(", ", EffectiveParameterNames);

                string SlicedCallableParametersText = CallableParameters.Count > 0
                        ? CallableParametersText + ", int32 MaxExecutionsPerFrame = 10"
                        : "int32 MaxExecutionsPerFrame = 10";

                string SlicedCallableArgumentsText = EffectiveParameterNames.Count > 0
                        ? "MaxExecutionsPerFrame, " + CallableArgumentsText
                        : "MaxExecutionsPerFrame";

                string BlueprintCallableBlock = string.Empty;

                if (HasMetadataFlag(EventMetadata, "BlueprintCallable"))
                {
                    BlueprintCallableBlock =
                        NewLine +
                        "\t// Auto-generated Blueprint broadcast wrappers." + NewLine +
                        "\tUFUNCTION(BlueprintCallable, Category = \"Events|" + EventName +
                        "\", meta = (DisplayName = \"Broadcast " + EventName + "\"))" + NewLine +
                        "\tvoid BP_Broadcast_" + EventName + "(" + CallableParametersText + ") {" + NewLine +
                        "\t\t" + EventName + ".Broadcast(" + CallableArgumentsText + ");" + NewLine +
                        "\t}" + NewLine +
                        NewLine +
                        "\tUFUNCTION(BlueprintCallable, Category = \"Events|" + EventName +
                        "\", meta = (DisplayName = \"Sliced Broadcast " + EventName +
                        "\", AdvancedDisplay = \"MaxExecutionsPerFrame\"))" + NewLine +
                        "\tvoid BP_SlicedBroadcast_" + EventName + "(" + SlicedCallableParametersText + ") {" + NewLine +
                        "\t\t" + EventName + ".SlicedBroadcast(" + SlicedCallableArgumentsText + ");" + NewLine +
                        "\t}" + NewLine;
                }

                string GeneratedEventSection =
                    "\t#pragma region Generated Blueprint Delegate for " +
                    EventName + NewLine +
                    "\tpublic:" + NewLine +
                    "\t" + DelegateMacroName +
                    "(F" + EventName + "BP" +
                    DelegateArgumentsText + ");" + NewLine +
                    "\tUPROPERTY(BlueprintAssignable, Category = \"Events\")" +
                    NewLine +
                    "\tF" + EventName + "BP " +
                    EventName + "_BP;" + NewLine +
                    "\tprivate:" + NewLine +
                    "\tuint8 _AutoBind_" + EventName +
                    " = [this]() -> uint8 {" + NewLine +
                    "\t\t" + EventName +
                    ".SubscribeLambda(this, [this](auto&&... args) {" +
                    NewLine +
                    "\t\t\t" + EventName +
                    "_BP.Broadcast(Forward<decltype(args)>(args)...);" +
                    NewLine +
                    "\t\t});" + NewLine +
                    "\t\treturn 0;" + NewLine +
                    "\t}();" + NewLine +
                    "\tpublic:" + NewLine +
                    BlueprintCallableBlock +
                    "\t#pragma endregion";

                GeneratedEventSections.Add(GeneratedEventSection);
            }

            const string GeneratedBeginMarker = "// YAE_GENERATED_CODE_BEGIN";
            const string GeneratedEndMarker = "// YAE_GENERATED_CODE_END";

            string GeneratedRegion =
                "\t" + GeneratedBeginMarker + NewLine +
                "\t#pragma region UEVENT Generated Code (DO NOT TOUCH!)" +
                NewLine +
                "\t// Generated by YetAnotherEventCodegen C#. " +
                "Do not edit this region by hand." + NewLine +
                "\t// Re-run the generator after changing UEVENT declarations." +
                NewLine +
                NewLine +
                string.Join(NewLine + NewLine, GeneratedEventSections) +
                NewLine +
                "\tprivate:" + NewLine +
                "\t#pragma endregion" + NewLine +
                "\t" + GeneratedEndMarker + NewLine;

            int GeneratedBeginIndex = HeaderText.IndexOf(GeneratedBeginMarker, StringComparison.Ordinal);
            int GeneratedEndIndex = HeaderText.IndexOf(GeneratedEndMarker, StringComparison.Ordinal);

            if ((GeneratedBeginIndex < 0) != (GeneratedEndIndex < 0))
            {
                throw new BuildException(
                    $"[YetAnotherEvent] Header '{RelativeHeaderPath}' contains only one " +
                    "YAE generated-code marker. Both markers must exist, or neither marker " +
                    "may exist."
                );
            }

            if (GeneratedBeginIndex < 0 && GeneratedEndIndex < 0)
            {
                int ClassClosingBraceIndex = FindOwningClassClosingBrace(HeaderText, HeaderOwningClassName);

                if (ClassClosingBraceIndex < 0)
                {
                    throw new BuildException($"[YetAnotherEvent] Could not locate the closing brace of class " +
                        $"'{HeaderOwningClassName}' in '{RelativeHeaderPath}'."
                    );
                }

                string InsertedHeaderText =
                    HeaderText.Substring(0, ClassClosingBraceIndex) +
                    NewLine +
                    GeneratedRegion +
                    HeaderText.Substring(ClassClosingBraceIndex);

                File.WriteAllText(HeaderPath, InsertedHeaderText);

                Console.WriteLine($"[YetAnotherEvent] Inserted generated UEVENT region into " +
                    $"'{HeaderOwningClassName}' in {HeaderPath}"
                );

                continue;
            }

            int GeneratedBeginLineStart = HeaderText.LastIndexOf('\n', GeneratedBeginIndex);

            GeneratedBeginLineStart = GeneratedBeginLineStart < 0
                    ? 0
                    : GeneratedBeginLineStart + 1;

            int GeneratedEndLineEnd = HeaderText.IndexOf('\n', GeneratedEndIndex + GeneratedEndMarker.Length);

            GeneratedEndLineEnd = GeneratedEndLineEnd < 0
                    ? HeaderText.Length
                    : GeneratedEndLineEnd + 1;

            string UpdatedHeaderText =
                HeaderText.Substring(0, GeneratedBeginLineStart) +
                GeneratedRegion +
                HeaderText.Substring(GeneratedEndLineEnd);

            if (!string.Equals(HeaderText, UpdatedHeaderText, StringComparison.Ordinal))
            {
                File.WriteAllText(HeaderPath, UpdatedHeaderText);

                Console.WriteLine($"[YetAnotherEvent] Updated generated UEVENT region: " +
                    HeaderPath
                );
            }
        }

        Console.WriteLine($"[YetAnotherEvent] C# scan complete. " +
            $"Checked {HeaderPaths.Count} headers and found " +
            $"{DetectedEventCount} UEVENT declarations."
        );

    }

    private static List<string> SplitTopLevelCommaSeparated(string Text)
    {
        List<string> Results = new List<string>();

        int StartIndex = 0;
        int AngleDepth = 0;
        int ParenthesisDepth = 0;
        int BracketDepth = 0;
        int BraceDepth = 0;

        bool bInsideString = false;
        bool bEscaped = false;

        for (int Index = 0; Index < Text.Length; ++Index)
        {
            char Character = Text[Index];

            if (bInsideString)
            {
                if (bEscaped)
                {
                    bEscaped = false;
                    continue;
                }

                if (Character == '\\')
                {
                    bEscaped = true;
                    continue;
                }

                if (Character == '"')
                {
                    bInsideString = false;
                }

                continue;
            }

            if (Character == '"')
            {
                bInsideString = true;
                continue;
            }

            switch (Character)
            {
                case '<':
                    ++AngleDepth;
                    break;
                case '>':
                    AngleDepth = Math.Max(0, AngleDepth - 1);
                    break;
                case '(':
                    ++ParenthesisDepth;
                    break;
                case ')':
                    ParenthesisDepth = Math.Max(0, ParenthesisDepth - 1);
                    break;
                case '[':
                    ++BracketDepth;
                    break;
                case ']':
                    BracketDepth = Math.Max(0, BracketDepth - 1);
                    break;
                case '{':
                    ++BraceDepth;
                    break;
                case '}':
                    BraceDepth = Math.Max(0, BraceDepth - 1);
                    break;
                case ',':
                    if (AngleDepth == 0 && ParenthesisDepth == 0 && BracketDepth == 0 && BraceDepth == 0)
                    {
                        string Item = Text.Substring(StartIndex, Index - StartIndex).Trim();

                        if (Item.Length > 0)
                        {
                            Results.Add(Item);
                        }

                        StartIndex = Index + 1;
                    }
                    break;
            }
        }

        string FinalItem = Text.Substring(StartIndex).Trim();

        if (FinalItem.Length > 0)
        {
            Results.Add(FinalItem);
        }

        return Results;
    }

    private static int FindOwningClassClosingBrace(string HeaderText, string OwningClassName)
    {
        string ClassPattern =
            @"\bclass\s+" +
            @"(?:(?:[A-Za-z_][A-Za-z0-9_]*_API)\s+)?" +
            Regex.Escape(OwningClassName) +
            @"\s*(?:final\s*)?:[^{]*\{";

        Match ClassMatch = Regex.Match(HeaderText, ClassPattern, RegexOptions.Singleline);
        if (!ClassMatch.Success) return -1;

        int OpenBraceIndex = HeaderText.LastIndexOf('{', ClassMatch.Index + ClassMatch.Length - 1);
        if (OpenBraceIndex < 0) return -1;

        return FindMatchingClosingBrace(HeaderText, OpenBraceIndex);
    }

    private static int FindMatchingClosingBrace(string Text, int OpenBraceIndex)
    {
        int BraceDepth = 0;

        bool bInsideString = false;
        bool bInsideCharacter = false;
        bool bInsideLineComment = false;
        bool bInsideBlockComment = false;
        bool bEscaped = false;

        for (int Index = OpenBraceIndex; Index < Text.Length; ++Index)
        {
            char Character = Text[Index];

            char NextCharacter = Index + 1 < Text.Length
                    ? Text[Index + 1]
                    : '\0';

            if (bInsideLineComment)
            {
                if (Character == '\n')
                {
                    bInsideLineComment = false;
                }

                continue;
            }

            if (bInsideBlockComment)
            {
                if (Character == '*' && NextCharacter == '/')
                {
                    bInsideBlockComment = false;
                    ++Index;
                }

                continue;
            }

            if (bInsideString)
            {
                if (bEscaped)
                {
                    bEscaped = false;
                    continue;
                }

                if (Character == '\\')
                {
                    bEscaped = true;
                    continue;
                }

                if (Character == '"')
                {
                    bInsideString = false;
                }

                continue;
            }

            if (bInsideCharacter)
            {
                if (bEscaped)
                {
                    bEscaped = false;
                    continue;
                }

                if (Character == '\\')
                {
                    bEscaped = true;
                    continue;
                }

                if (Character == '\'')
                {
                    bInsideCharacter = false;
                }

                continue;
            }

            if (Character == '/' && NextCharacter == '/')
            {
                bInsideLineComment = true;
                ++Index;
                continue;
            }

            if (Character == '/' && NextCharacter == '*')
            {
                bInsideBlockComment = true;
                ++Index;
                continue;
            }

            if (Character == '"')
            {
                bInsideString = true;
                continue;
            }

            if (Character == '\'')
            {
                bInsideCharacter = true;
                continue;
            }

            if (Character == '{')
            {
                ++BraceDepth;
                continue;
            }

            if (Character == '}')
            {
                --BraceDepth;

                if (BraceDepth == 0)
                {
                    return Index;
                }
            }
        }

        return -1;
    }

    private static string FindOwningReflectedClass(string HeaderText, int EventIndex)
    {
        int SearchIndex = EventIndex;

        while (SearchIndex >= 0)
        {
            int UClassIndex = HeaderText.LastIndexOf("UCLASS", SearchIndex, StringComparison.Ordinal);

            if (UClassIndex < 0)
            {
                return string.Empty;
            }

            string CandidateText = HeaderText.Substring(UClassIndex, EventIndex - UClassIndex);

            Match ClassMatch = Regex.Match(
                CandidateText,
                @"\bclass\s+" +
                @"(?:(?:[A-Za-z_][A-Za-z0-9_]*_API)\s+)?" +
                @"(?<ClassName>[A-Za-z_][A-Za-z0-9_]*)" +
                @"\s*(?:final\s*)?:",
                RegexOptions.Singleline
            );

            if (ClassMatch.Success)
            {
                return ClassMatch.Groups["ClassName"].Value;
            }

            SearchIndex = UClassIndex - 1;
        }

        return string.Empty;
    }

    private static bool HasMetadataFlag(string Metadata, string FlagName)
    {
        foreach (string MetadataItem in SplitTopLevelCommaSeparated(Metadata))
        {
            if (string.Equals(MetadataItem.Trim(), FlagName, StringComparison.Ordinal))
            {
                return true;
            }
        }

        return false;
    }

    private static void RejectUnsupportedNetworkMetadata(string RelativeHeaderPath, string OwningClassName, string EventName, string Metadata)
    {
        string[] NetworkFlags = { "Networkable", "ClientOnly", "ServerOnly" };
        List<string> RequestedNetworkFlags = new List<string>();

        foreach (string NetworkFlag in NetworkFlags)
        {
            if (HasMetadataFlag(Metadata, NetworkFlag))
            {
                RequestedNetworkFlags.Add(NetworkFlag);
            }
        }

        if (RequestedNetworkFlags.Count == 0) return;

        string RequestedFlagsText = string.Join(", ", RequestedNetworkFlags);

        throw new BuildException(
            $"[YetAnotherEvent] UEVENT '{OwningClassName}::{EventName}' in " +
            $"'{RelativeHeaderPath}' requests unsupported legacy network metadata: " +
            $"{RequestedFlagsText}. The C# generator intentionally does not reproduce " +
            "the old automatic RPC wrappers because their ownership, authority, reliability, " +
            "and calling-direction semantics have not been redesigned yet. Remove the network " +
            "flag for now; networking will return as an explicit opt-in feature."
        );
    }

    private static void ValidateExplicitParameterNames(string RelativeHeaderPath, string OwningClassName,
        string EventName, List<string> ParameterNames, int ParameterTypeCount, bool bGeneratesBlueprintCallables)
    {
        Regex IdentifierPattern = new Regex(@"^[A-Za-z_][A-Za-z0-9_]*$");
        HashSet<string> UsedParameterNames = new HashSet<string>(StringComparer.Ordinal);
        int UsedNameCount = Math.Min(ParameterNames.Count, ParameterTypeCount);

        for (int Index = 0; Index < UsedNameCount; ++Index)
        {
            string ParameterName = ParameterNames[Index];

            if (!IdentifierPattern.IsMatch(ParameterName))
            {
                throw new BuildException(
                    $"[YetAnotherEvent] UEVENT '{OwningClassName}::{EventName}' in " +
                    $"'{RelativeHeaderPath}' uses invalid ParamNames entry " +
                    $"'{ParameterName}'. Parameter names must be valid C++ identifiers."
                );
            }

            if (!UsedParameterNames.Add(ParameterName))
            {
                throw new BuildException(
                    $"[YetAnotherEvent] UEVENT '{OwningClassName}::{EventName}' in " +
                    $"'{RelativeHeaderPath}' uses duplicate parameter name " +
                    $"'{ParameterName}'. Generated delegate and function parameter names " +
                    "must be unique."
                );
            }

            if (
                bGeneratesBlueprintCallables &&
                string.Equals(ParameterName, "MaxExecutionsPerFrame", StringComparison.Ordinal)
            )
            {
                throw new BuildException(
                    $"[YetAnotherEvent] UEVENT '{OwningClassName}::{EventName}' in " +
                    $"'{RelativeHeaderPath}' uses reserved generated parameter name " +
                    "'MaxExecutionsPerFrame'. Choose another ParamNames entry because the " +
                    "sliced Blueprint wrapper adds that parameter automatically."
                );
            }
        }
    }

    private static void EmitParameterTypeWarnings(string RelativeHeaderPath, string OwningClassName, string EventName, List<string> ParameterTypes)
    {
        string[] ObviouslyUnsupportedTypeTokens = {
            "TFunction",
            "TUniquePtr",
            "TSharedPtr",
            "TSharedRef",
            "TEvent<"
        };

        foreach (string ParameterType in ParameterTypes)
        {
            string CompactType = Regex.Replace(ParameterType, @"\s+", string.Empty);

            foreach (string UnsupportedTypeToken in ObviouslyUnsupportedTypeTokens)
            {
                if (CompactType.Contains(UnsupportedTypeToken, StringComparison.Ordinal))
                {
                    WriteGeneratorWarning(RelativeHeaderPath, OwningClassName, EventName,
                        $"parameter type '{ParameterType}' looks unsupported for generated " +
                        "Blueprint dynamic delegates and UFUNCTION wrappers."
                    );

                    break;
                }
            }

            string NormalizedType = Regex.Replace(ParameterType, @"\bconst\b", string.Empty);
            NormalizedType = NormalizedType.Replace("&", string.Empty).Trim();

            if (string.Equals(NormalizedType, "int", StringComparison.Ordinal))
            {
                WriteGeneratorWarning(RelativeHeaderPath, OwningClassName, EventName,
                    "parameter type 'int' should be changed to 'int32' for Unreal-reflected " +
                    "generated Blueprint signatures."
                );
            }

            if (CompactType.Contains("&&", StringComparison.Ordinal))
            {
                WriteGeneratorWarning(RelativeHeaderPath, OwningClassName, EventName,
                    $"parameter type '{ParameterType}' is an rvalue reference. Generated " +
                    "UFUNCTION and dynamic delegate wrappers are unlikely to expose it correctly."
                );
            }
        }
    }

    private static void WriteGeneratorWarning(string RelativeHeaderPath, string OwningClassName, string EventName, string Message)
    {
        Console.WriteLine($"[YetAnotherEvent][Warning] {RelativeHeaderPath}: " +
            $"{OwningClassName}::{EventName} {Message}"
        );
    }

    private static string GetDelegateMacroName(int ParameterCount)
    {
        switch (ParameterCount)
        {
            case 0:
                return "DECLARE_DYNAMIC_MULTICAST_DELEGATE";
            case 1:
                return "DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam";
            case 2:
                return "DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams";
            case 3:
                return "DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams";
            case 4:
                return "DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams";
            case 5:
                return "DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams";
            case 6:
                return "DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams";
            case 7:
                return "DECLARE_DYNAMIC_MULTICAST_DELEGATE_SevenParams";
            case 8:
                return "DECLARE_DYNAMIC_MULTICAST_DELEGATE_EightParams";
            case 9:
                return "DECLARE_DYNAMIC_MULTICAST_DELEGATE_NineParams";
            default:
                throw new BuildException($"[YetAnotherEvent] Dynamic multicast delegates with " +
                    $"{ParameterCount} parameters are not supported."
                );
        }
    }
}