import pydemangler
import re

# if pydemangler isnt found , pip install git+https://github.com/wbenny/pydemangler.git

INPUT_FILE = "C:/Users/lucas/Desktop/Telltale/WDC.asm" # in IDA File -> Produce File -> Create ASM file
OUTPUT_FILE = "../../Dev/Tools/SymbolExtract/Latest.TXT"

def extract_symbols(filename):
    results = []
    symbol_line_re = re.compile(r"^; Symbol\s+(?P<symbol>.+)$")
    hash_re = re.compile(r"<([0-9A-Fa-f]+)h>")

    with open(filename, "r", encoding="utf-8", errors="ignore") as f:
        previous_symbol = None

        for line in f:
            # Check for symbol comment line
            symbol_match = symbol_line_re.match(line.strip())
            if symbol_match:
                previous_symbol = symbol_match.group("symbol").strip()
                continue
            
            # If we just saw a symbol line, check this line for the hash
            if previous_symbol:
                hash_match = hash_re.search(line)
                if hash_match:
                    hex_hash = "0x" + hash_match.group(1)
                    results.append((previous_symbol, hex_hash))
                    previous_symbol = None  # Reset for next symbol

    return results

symbols = extract_symbols(INPUT_FILE)
symbols.sort(key=lambda x: x[0].lower())  # Case-insensitive sort

# Write output
with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    for symbol, hash_ in symbols:
        symbold = pydemangler.demangle(symbol)
        if symbold == None:
            symbold = symbol
        f.write(f"{symbol} Symbol<{hash_}>\n")
        print(f"- found {symbol}")

print(f"- done")