import os
import re

PATTERN = re.compile( # compile text pattern into object once
	r"(UEVENT\s*\(\)\s*TEvent<(.*?)>\s+(\w+)(?:{})?;)"
    r"(?:\s*#pragma region Generated Blueprint Delegate.*?\s*#pragma endregion)?",
    re.DOTALL
)

def process_uevents(file_path):
    with open(file_path, 'r') as file:
        content = file.read()

    # instantly stop, if there's no 'UEVENT' -> since Regex is computationally heavy
    if "UEVENT" not in content:
        return

    def replacer(match):
        original_declaration = match.group(1)
        types = match.group(2).strip()
        name = match.group(3)

        type_list = [t.strip() for t in types.split(',') if t.strip()]
        if len(type_list) == 1 and type_list[0] == "": # If the Event is 'TEvent<>': Don't accidently create a list with one empty string
            type_list = []

        param_count = len(type_list)
        suffixes = ["", "_OneParam", "_TwoParams", "_ThreeParams", "_FourParams", "_FiveParams", "_SixParams", "_SevenParams", "_EightParams", "_NineParams"]
        suffix = suffixes[param_count] if param_count < len(suffixes) else "_TooManyParams" # Limit to a maximum of 9 parameters
        
        delegate_name = f"F{name}BP"
        signature = f"DECLARE_DYNAMIC_MULTICAST_DELEGATE{suffix}({delegate_name}"

        for i, t in enumerate(type_list):
            signature += f", {t}, Param{i+1}" # e.g., float, Param1, int32, Param2
        
        signature += ");"

        # Use IIFE to execute subscription during object construction
        generated_block = (
            f"\n\t#pragma region Generated Blueprint Delegate for {name}\n"
            f"\t{signature}\n"
            f"\tUPROPERTY(BlueprintAssignable, meta = (ToolTip = \"WARNING: Do not bind via the Details Panel for external actors (causes memory stacking). Use the 'Bind Event' node in BeginPlay instead.\"), Category = \"Events\")\n"
            f"\t{delegate_name} {name}_BP;\n"
            f"\tprivate:\n"
            f"\tuint8 _AutoBind_{name} = [this]() -> uint8 {{\n"
            f"\t\t{name}.SubscribeLambda([this](auto&&... args) {{\n"
            f"\t\t\t{name}_BP.Broadcast(Forward<decltype(args)>(args)...);\n"
            f"\t\t}});\n"
            f"\t\treturn 0;\n"
            f"\t}}();\n"
            f"\tpublic:\n"
            f"\t#pragma endregion"
        )

        return original_declaration + generated_block

    new_content = PATTERN.sub(replacer, content)

    if new_content != content:
        with open(file_path, 'w') as file:
            file.write(new_content)
        print(f"Processed UEVENTs in: {os.path.basename(file_path)}")

source_dir = os.path.join(os.path.dirname(__file__), "Source")
# Go through every folder and sub-folder
for root, _, files in os.walk(source_dir):
    for file_name in files:
        if file_name.endswith(".h"):
            process_uevents(os.path.join(root, file_name))