import struct
import json

# PARAMETERS ARE NOT PASSED INTO THE SCRIPT, MODIFY EVERYTHING HERE.

CONFIG = {
    "DUMP_FILE": "C:/Users/lucas/Desktop/Telltale/All Vers/_MEMDMPS_FOR_32/bone1_v20late.bin", # INPUT FILE (CHANGME)
    "BASE_OFFSET": 10434040, # note this is NOT the g_MetaClases array from the constructor, thats the pointer pointer, deref it (go to the value at the offset, and that offset is this)
    "PTR_SIZE": 4,
    "NEXT_OFFSET": 168,
    "OUTPUT_JS": "../../Dev/Tests/Traverse/Latest.JSON",
    "MAX_CLASSES": 3000,
    "BITFLAGS": {
        "flags": {
            0x01: "Registered",
        },
        "Members.flags": {
            0x01: "SerialiseDisable",
            0x02: "BlockingDisable",
            0x10: "BaseClass",
            0x40: "EnumWrapper"
        }
        ## eg Members.flags to etc etc
    },

    # Define struct members to print
    # Format: field_name: (offset, type)
    # Type can be: 'string', 'int'
    "FIELDS": {
        "description": (12, "string"),
        "typeinfo name": (16, "string"),
        "extension": (8, "string"),
        "size": (28, "int"),
        "flags": (4, "int"),
        "SerialisedVersionInfo": {
            "type": "array",
            "offset": 164,
            "next_offset": -1, # single pointer not a linked list
            "fields": {
                "VersionCRC": (56, "int"),
                "IsBlocked": (64, "bool")
            }
        },
        "Members": {
            "type": "array",
            "offset": 160,
            "next_offset": 24,
            "fields": {
                "name": (4, "string"),
                "flags": (16, "int"),
                "member_offset": (12, "int"),
                "membertype": {
                    "offset": 8,
                    "next_offset": -1,
                    "type": "array",
                    "fields": { # another MCD
                        "type_name": (4, "string")
                    }
                } # enum descriptors too but wont bother
            }
        }
        # Add more fields here as needed
    }
}

# === Memory Utilities ===

def read_pointer(f, addr):
    f.seek(addr)
    return struct.unpack('<I', f.read(CONFIG["PTR_SIZE"]))[0]

def read_int(f, addr):
    f.seek(addr)
    return struct.unpack('<I', f.read(4))[0]

def read_cstring(f, addr, max_len=256):
    if addr == 0:
        return None
    f.seek(addr)
    data = bytearray()
    while len(data) < max_len:
        byte = f.read(1)
        if not byte or byte == b'\x00':
            break
        data += byte
    return data.decode(errors='replace')

def read_field(f, base_addr, offset, field_type, field_path=""):
    addr = base_addr + offset
    if field_type == "int":
        value = read_int(f, addr)
        bitflags = CONFIG.get("BITFLAGS", {}).get(field_path)
        if bitflags:
            named_flags = [name for bit, name in bitflags.items() if value & bit]
            unknown_bits = value & ~sum(bitflags.keys())
            if unknown_bits:
                named_flags.append(f"0x{unknown_bits:X}")
            return " | ".join(named_flags) if named_flags else "0x0"
        return f"0x{value:X}"
    elif field_type == "string":
        ptr = read_pointer(f, addr)
        return read_cstring(f, ptr) or "<None>"
    elif field_type == "bool":
        f.seek(base_addr + offset)
        value = struct.unpack('<B', f.read(1))[0]
        return "True" if value else "False"
    else:
        return "<unsupported>"

def read_struct(f, base_addr, field_defs, path=""):
    struct_data = {}
    for fname, config in field_defs.items():
        field_path = f"{path}.{fname}" if path else fname
        if isinstance(config, tuple):
            offset, ftype = config
            struct_data[fname] = read_field(f, base_addr, offset, ftype, field_path)
        elif isinstance(config, dict) and config.get("type") == "array":
            nested = read_struct_list(
                f,
                base_addr + config["offset"],
                config["next_offset"],
                config["fields"],
                path=field_path
            )
            struct_data[fname] = nested
        else:
            struct_data[fname] = "<unsupported>"
    return struct_data

def read_struct_list(f, start_ptr_addr, next_offset, field_defs, path=""):
    results = []
    ptr = read_pointer(f, start_ptr_addr)
    if ptr == 0:
        return results

    seen = set()
    current = ptr

    while current != 0 and current not in seen:
        seen.add(current)
        results.append(read_struct(f, current, field_defs, path))
        if next_offset == -1:
            break
        current = read_pointer(f, current + next_offset)

    return results

def flatten_struct(row_data, field_order):
    flat_row = []
    for key in field_order:
        val = row_data[key]
        if isinstance(val, list):
            display = "; ".join(
                f"{' | '.join(f'{k}={v}' for k, v in entry.items())}"
                for entry in val
            )
            flat_row.append(display or "<empty>")
        else:
            flat_row.append(str(val))
    return flat_row

def print_table(headers, rows):
    col_widths = [max(len(str(cell)) for cell in col) for col in zip(*([headers] + rows))]
    header_line = " | ".join(f"{headers[i]:<{col_widths[i]}}" for i in range(len(headers)))
    separator = "-+-".join("-" * col_widths[i] for i in range(len(headers)))
    print(header_line)
    print(separator)
    for row in rows:
        print(" | ".join(f"{row[i]:<{col_widths[i]}}" for i in range(len(row))))

def write_json(file_path, data):
    with open(file_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)

def main():
    structured_output = []

    with open(CONFIG["DUMP_FILE"], 'rb') as f:
        current = CONFIG["BASE_OFFSET"]
        seen = set()
        count = 0

        while current != 0 and current not in seen and count < CONFIG["MAX_CLASSES"]:
            seen.add(current)
            base_data = read_struct(f, current, CONFIG["FIELDS"])
            structured_output.append({
                "Index": count,
                "Address": f"0x{current:08X}",
                "Data": base_data
            })
            current = read_pointer(f, current + CONFIG["NEXT_OFFSET"])
            count += 1

    output_path = CONFIG["OUTPUT_JS"]
    if output_path != None:
        write_json(output_path, structured_output)
        print(f"- dump json: {output_path}")


if __name__ == "__main__":
    main()