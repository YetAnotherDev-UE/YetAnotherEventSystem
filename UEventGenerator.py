import os
import re

PATTERN = re.compile( # compile text pattern into object once
	r"(UEVENT\s*\((.*?)\)\s*TEvent<(.*?)>\s+(\w+)(?:\{\})?;)"
    r"(?:\s*#pragma region Generated Blueprint Delegate.*?\s*#pragma endregion)?",
    re.DOTALL
)

def process_uevents(file_path):
    with open(file_path, 'r') as file:
        content = file.read()

    # instantly stop, if there's no 'UEVENT' -> since Regex is computationally heavy
    if "UEVENT" not in content:
        return

    # Automatically detect if this class is network-capable (only actors and actor components are)
    is_network_capable = any(base in content for base in ["public AActor", "public UActorComponent"])

    def replacer(match):
        original_declaration = match.group(1)
        uevent_args = match.group(2).strip()
        types = match.group(3).strip()
        name = match.group(4)

        # Function to safely split commas, ignoring commas inside nested < > brackets (e.g., TMap<int32, float>)
        def safe_split_types(type_str):
            result = []
            current = ""
            depth = 0;
            for char in type_str:
                if char == '<':
                    depth += 1
                elif char == '>':
                    depth -= 1

                if char == ',' and depth == 0:
                    result.append(current.strip())
                    current = ""
                else:
                    current += char
            if current.strip():
                result.append(current.strip())
            return result

        type_list = safe_split_types(types)
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
        
        call_params = ", ".join([f"{t} Param{i+1}" for i, t in enumerate(type_list)])
        call_args = ", ".join([f"Param{i+1}" for i in range(len(type_list))])

        sliced_params = f"{call_params}, int32 MaxExecutionsPerFrame = 10" if call_params else "int32 MaxExecutionsPerFrame = 10"
        sliced_args = f"MaxExecutionsPerFrame, {call_args}" if call_args else "MaxExecutionsPerFrame"
        # Don't use default values in RPCs to avoid UHT issues
        rpc_sliced_params = f"{call_params}, int32 MaxExecutionsPerFrame" if call_params else "int32 MaxExecutionsPerFrame"

        bp_callable_block = ""
        if "BlueprintCallable" in uevent_args:
            bp_callable_block = (
                f"\t// Auto-Generated Blueprint Callables\n"
                f"\tUFUNCTION(BlueprintCallable, Category = \"Events|{name}\", meta = (DisplayName = \"Broadcast {name}\"))\n"
                f"\tvoid BP_Broadcast_{name}({call_params}) {{\n"
                f"\t\t{name}.Broadcast({call_args});\n"
                f"\t}}\n\n"
                f"\tUFUNCTION(BlueprintCallable, Category = \"Events|{name}\", meta = (DisplayName = \"Sliced Broadcast {name}\", AdvancedDisplay = \"MaxExecutionsPerFrame\"))\n"
                f"\tvoid BP_SlicedBroadcast_{name}({sliced_params}) {{\n"
                f"\t\t{name}.SlicedBroadcast({sliced_args});\n"
                f"\t}}\n\n"
            )

        network_block = ""
        interceptor_logic = ""

        if is_network_capable:
            # Determine the Remote Procedure Call (RPC) type based on the UEVENT arguments
            rpc_type = None
            prefix = ""
            if "Networkable" in uevent_args:
                rpc_type = "NetMulticast"
                prefix = "Multicast_"
            elif "ClientOnly" in uevent_args:
                rpc_type = "Client"
                prefix = "Client_"
            elif "ServerOnly" in uevent_args:
                rpc_type = "Server"
                prefix = "Server_"
            
            # Generate the RPC wrapper if a valid argument was found
            if rpc_type:
                rpc_params = ", ".join([f"{t} Param{i+1}" for i, t in enumerate(type_list)])
                rpc_args = ", ".join([f"Param{i+1}" for i in range(len(type_list))])
            
                network_block = (
                    f"\t// Auto-Generated {rpc_type} RPC Wrapper\n"
                    f"\tUFUNCTION({rpc_type}, Reliable)\n"
                    f"\tvoid {prefix}{name}({call_params});\n"
                    f"\tvirtual void {prefix}{name}_Implementation({call_params}) {{\n"
                    f"\t\t{name}.Broadcast({call_args});\n"
                    f"\t}}\n\n"
                    f"\t// Auto-Generated {rpc_type} Sliced RPC Wrapper\n"
                    f"\tUFUNCTION({rpc_type}, Reliable)\n"
                    f"\tvoid {prefix}Sliced_{name}({rpc_sliced_params});\n"
                    f"\tvirtual void {prefix}Sliced_{name}_Implementation({rpc_sliced_params}) {{\n"
                    f"\t\t{name}.SlicedBroadcast({sliced_args});\n"
                    f"\t}}\n\n"
                )
            
                # Inject the interceptor to route the .Broadcast() over the network
                interceptor_sliced_call = f"{prefix}Sliced_{name}(Forward<decltype(args)>(args)..., MaxExecutionsPerFrame);" if call_args else f"{prefix}Sliced_{name}(MaxExecutionsPerFrame);"

                interceptor_logic = (
                    f"\t\t{name}.NetworkInterceptor = [this](auto&&... args) -> bool {{\n"
                    f"\t\t\tif (this->HasAuthority()) {{\n"
                    f"\t\t\t\t{prefix}{name}(Forward<decltype(args)>(args)...);\n"
                    f"\t\t\t\treturn true;\n"
                    f"\t\t\t}}\n"
                    f"\t\t\treturn false;\n"
                    f"\t\t}};\n"
                    f"\t\t{name}.SlicedNetworkInterceptor = [this](int32 MaxExecutionsPerFrame, auto&&... args) -> bool {{\n"
                    f"\t\t\tif (this->HasAuthority()) {{\n"
                    f"\t\t\t\t{interceptor_sliced_call}\n"
                    f"\t\t\t\treturn true;\n"
                    f"\t\t\t}}\n"
                    f"\t\t\treturn false;\n"
                    f"\t\t}};\n"
                )
        else:
            if "Networkable" in uevent_args or "ClientOnly" in uevent_args or "ServerOnly" in uevent_args:
                print(f"WARNING: Skipping network generation for {name} in {os.path.basename(file_path)}: Class is not an AActor or UActorComponent.")
                interceptor_logic = ""

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
            f"{interceptor_logic}"
            f"\t\treturn 0;\n"
            f"\t}}();\n"
            f"\tpublic:\n"
            f"{bp_callable_block}"
            f"{network_block}"
            f"\t#pragma endregion"
        )

        return original_declaration + generated_block

    new_content = PATTERN.sub(replacer, content)

    if new_content != content:
        with open(file_path, 'w') as file:
            file.write(new_content)
        print(f"Processed UEVENTs in: {os.path.basename(file_path)}")

source_dir = os.path.join(os.path.dirname(__file__), "Source") # Go through every folder and sub-folder
for root, _, files in os.walk(source_dir):
    for file_name in files:
        if file_name.endswith(".h"):
            process_uevents(os.path.join(root, file_name))