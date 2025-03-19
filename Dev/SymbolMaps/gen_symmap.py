import sys

def gfn(path):
    return path.strip().replace('\\', '/').split('/')[-1]

def gen_symmap(input_file, bCRC32, game):
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        output_lines = []
        for line in lines:
            stripped_line = line.strip()
            filename = gfn(stripped_line)
            print(f"PROCESSING: {filename}")

            if filename:
                if bCRC32:
                    output_lines.append(f"CRC32:{filename}\n")
                else:
                    output_lines.append(f"{filename}\n")
        
        output_file = "Files_" + game + ".symmap"
        with open(output_file, 'w', encoding='utf-8') as f:
            f.writelines(output_lines)
        
        print(f"Generated symmap to {output_file}")
    except Exception as e:
        print(f"err: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python symmap.py <input_file> <game> <crc32 yes/no>")
    else:
        gen_symmap(sys.argv[1], sys.argv[3].lower() == "yes", sys.argv[2])
