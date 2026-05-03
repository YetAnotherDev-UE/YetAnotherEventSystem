import os
import re
import json
import hashlib

PATTERN = re.compile( # compile text pattern into object once
	r"(UEVENT\s*\(((?:[^()]*|\([^()]*\))*)\)\s*(?:(?:mutable|const|static)\s+)*TEvent<(.*?)>\s+(\w+)\s*(?:\{\})?\s*;)"
    r"(?:\s*#pragma region Generated Blueprint Delegate.*?\s*#pragma endregion)?",
    re.DOTALL
)

def get_project_root():
    current_dir = os.path.abspath(os.path.dirname(__file__))
    while current_dir != os.path.dirname(current_dir):
        if any(f.endswith('.uproject') for f in os.listdir(current_dir)):
            return current_dir
        current_dir = os.path.dirname(current_dir)
    return os.path.abspath(os.path.dirname(__file__))

PROJECT_ROOT = get_project_root()

# Move cache to 'Intermediate' folder (ignored by source control)
INTERMEDIATE_DIR = os.path.join(PROJECT_ROOT, "Intermediate", "EventSystem")
if not os.path.exists(INTERMEDIATE_DIR):
    os.makedirs(INTERMEDIATE_DIR)

CACHE_FILE = os.path.join(INTERMEDIATE_DIR, "event_cache.json")

def load_cache():
    if os.path.exists(CACHE_FILE):
        try:
            with open(CACHE_FILE, 'r', encoding='utf-8') as f:
                return json.load(f)
        except: return {}
    return {}

def save_cache(cache):
    with open(CACHE_FILE, 'w', encoding='utf-8') as f:
        json.dump(cache, f, indent=4)

def sanitize_type_to_name(type_str):
    # Converts 'const FString&' to "String"
    # Remove modifiers
    clean = type_str.replace("const", "").replace("&", "").replace("*", "").strip()

    # Turn 'TScriptInterface' to the inner type
    script_interface_match = re.match(r"TScriptInterface<(.*?)>", clean)
    if script_interface_match:
        clean = script_interface_match.group(1).strip()

    blueprint_names = {
        "int32": "Int",
        "int64": "Int64",
        "float": "Float",
        "double": "Double",
        "bool": "Bool",
        "FString": "String",
        "FName": "Name",
        "FText": "Text",
        "FVector": "Location",
        "FRotator": "Rotation",
        "FTransform": "Transform",
        "AActor*": "Actor",
        "UObject*": "Object"
    }

    if clean in blueprint_names:
        return blueprint_names[clean]

    # Remove Prefixes (only if followed by another uppercase letter)
    if len(clean) > 1 and clean[0] in "FUATSI" and clean[1].isupper():
        clean = clean[1:]

    # Remove template brackets (E.g., TMap<int32, FString> -> Map_Int32_String)
    clean = clean.replace("<", "_").replace(">", "").replace(",", "_").replace("::", "_")
    return clean[:1].upper() + clean[1:]

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

def process_uevents(file_path, cached_hash):
    with open(file_path, 'r', encoding='utf-8', errors='replace') as file:
        original_content = file.read()

    # Fast-Fail: No events and no old code to clean up
    if "UEVENT" not in original_content and "UEVENT Generated Code" not in original_content:
        return False, ""

    # Clean up regions (if run multiple times)
    content = re.sub(r"\s*#pragma region UEVENT Generated Code \(DO NOT TOUCH!\).*?\s*private:\s*#pragma endregion\s*", "\n", original_content, flags=re.DOTALL)

    matches = list(PATTERN.finditer(content))
    event_fingerprint = "".join(m.group(0) for m in matches) # Create a unique string from the signature
    current_hash = hashlib.md5(event_fingerprint.encode('utf-8')).hexdigest()

    # Fast-Fail: Abort, if events haven't changed
    if current_hash == cached_hash:
        return False, current_hash

    # NOTE: At this point we know the events actually changed

    is_network_capable = any(base in original_content for base in ["public AActor", "public UActorComponent"])
    generated_blocks = [] 

    for match in matches:
        # Supported arguments
        # ServerOnly (Client-to-Server): E.g., a player clicks their mouse to shoot, the client asks the server to fire the weapon. Ran by only the Server
        # ClientOnly (Server-to-Owning-Client): E.g., the server calculates player damage and wants the player to show a red vignette. Ran only be the specific client who owns the actor
        # Networkable/Multicast (Server-to-Everyone): E.g., An explosion, where everyone needs to see the visual effect. Ran by the server and all connected clients
        full_macro = match.group(1)
        uevent_args = match.group(2).strip()
        types = match.group(3).strip()
        name = match.group(4)

        type_list = safe_split_types(types)
        if len(type_list) == 1 and type_list[0] == "": # If the Event is 'TEvent<>': Don't accidently create a list with one empty string
            type_list = []

        custom_names = []
        name_match = re.search(r'ParamNames\s*=\s*\((.*?)\)', uevent_args)
        if name_match:
            raw_names = name_match.group(1).split(',')
            custom_names = [n.strip().strip('"').strip("'") for n in raw_names]

        # Type-based default names
        sanitized_bases = [sanitize_type_to_name(t) for t in type_list]
        total_type_counts = {}
        for b in sanitized_bases:
            total_type_counts[b] = total_type_counts.get(b, 0) + 1

        current_type_counts = {}
        final_names = []
        for i, t in enumerate(type_list):
            # Use custom name if provided
            if i < len(custom_names):
                final_names.append(custom_names[i])
            else:
                base = sanitized_bases[i]
                # If there's more than one of this type, add index
                if total_type_counts[base] > 1:
                    current_type_counts[base] = current_type_counts.get(base, 0) + 1
                    final_names.append(f"{base}{current_type_counts[base]}")
                else:
                    final_names.append(base)

        param_count = len(type_list)
        suffixes = ["", "_OneParam", "_TwoParams", "_ThreeParams", "_FourParams", "_FiveParams", "_SixParams", "_SevenParams", "_EightParams", "_NineParams"]
        suffix = suffixes[param_count] if param_count < len(suffixes) else "_TooManyParams" # Limit to a maximum of 9 parameters
        
        delegate_name = f"F{name}BP" # 'F' standard prefix for structs/delegates
        signature = f"DECLARE_DYNAMIC_MULTICAST_DELEGATE{suffix}({delegate_name}"

        for i, t in enumerate(type_list):
            signature += f", {t}, {final_names[i]}" # e.g., float, Param1, int32, Param2 
        signature += ");"

        call_params = ", ".join([f"{type_list[i]} {final_names[i]}" for i in range(len(type_list))])
        call_args = ", ".join(final_names)

        sliced_params = f"{call_params}, int32 MaxExecutionsPerFrame = 10" if call_params else "int32 MaxExecutionsPerFrame = 10"
        sliced_args = f"MaxExecutionsPerFrame, {call_args}" if call_args else "MaxExecutionsPerFrame"
        rpc_sliced_params = f"int32 MaxExecutionsPerFrame, {call_params}" if call_params else "int32 MaxExecutionsPerFrame" # Don't use default values in RPCs to avoid UHT issues

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

                # Inject the interceptor to route the .Broadcast() over the network
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

        # Use IIFE to subscribe blueprint event call
        generated_blocks.append(
            f"\n\t#pragma region Generated Blueprint Delegate for {name}\n"
            f"\tpublic:\n"
            f"\t{signature}\n"
            f"\tUPROPERTY(BlueprintAssignable, Category = \"Events\")\n"
            f"\t{delegate_name} {name}_BP;\n"
            f"\tprivate:\n"
            f"\tuint8 _AutoBind_{name} = [this]() -> uint8 {{\n"
            f"\t\t{name}.SubscribeLambda(this, [this](auto&&... args) {{\n"
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


    if generated_blocks:
        master_block = "\n\t#pragma region UEVENT Generated Code (DO NOT TOUCH!)\n\n"
        master_block += "\n".join(generated_blocks)
        master_block += "\n\tprivate:\n"
        master_block += "\t#pragma endregion\n"

        # Inject before the final '}''
        last_brace_idx = content.rfind('};')
        if last_brace_idx != -1:
            content = content[:last_brace_idx] + master_block + content[last_brace_idx:]
        else:
            print(f"WARNING: Could not find closing class brace in {os.path.basename(file_path)}")

    # Only write to the file, if the content actually changed
    if content != original_content:
        with open(file_path, 'w', encoding='utf-8') as file:
            file.write(content)
        print(f"Processed UEVENTs in: {os.path.basename(file_path)}")
        return True, current_hash

    return False, current_hash

def main():
    source_dir = os.path.join(PROJECT_ROOT, "Source")

    cache = load_cache()
    updated_cache = {}
    files_processed = 0

    for root, _, files in os.walk(source_dir):
        for file_name in files:
            if file_name.endswith(".h"):
                abs_path = os.path.join(root, file_name)
                rel_path = os.path.relpath(abs_path, PROJECT_ROOT).replace('\\', '/')
                current_mtime = os.path.getmtime(abs_path)

                # Get the data (if exists)
                file_cache_data = cache.get(rel_path, {"mtime": 0, "hash": ""})

                if file_cache_data["mtime"] == current_mtime:
                    updated_cache[rel_path] = file_cache_data
                    continue

                # File was modified -> run parser
                was_modified, new_hash = process_uevents(abs_path, file_cache_data["hash"])

                # Update cache
                final_mtime = os.path.getmtime(abs_path) if was_modified else current_mtime
                updated_cache[rel_path] = {"mtime": final_mtime, "hash": new_hash}
                files_processed += 1

    save_cache(updated_cache)

    if files_processed > 0:
        print(f"UEVENT scan complete. Checked {files_processed} modified files.")
    else:
        print("UEVENT scan complete. No files were modified since the last run.")

if __name__ == "__main__":
    main()