import sys
import os
import argparse

def format_hex_dump(file_path, base_addr=0x000000090E630F8E0):
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        return

    try:
        with open(file_path, 'rb') as f:
            offset = 0
            while True:
                chunk = f.read(16)
                if not chunk:
                    break
                
                # 1. Format Addresses: [Absolute][Relative]
                abs_addr_str = f"[0x{base_addr + offset:016X}]"
                rel_off_str = f"[{offset:016X}:"
                
                # 2. Format Hex Bytes (lowercase, space-separated)
                hex_part = " ".join(f"{b:02x}" for b in chunk)
                
                # Padding for the last line if it's shorter than 16 bytes
                if len(chunk) < 16:
                    hex_part = hex_part.ljust(47) # 16*2 hex + 15 spaces
                
                # 3. Format ASCII Sidebar
                # Replace non-printable chars (0-31, 127+) with a dot
                ascii_part = "".join(chr(b) if 32 <= b <= 126 else "." for b in chunk)
                
                # Print the full line
                print(f"{abs_addr_str}{rel_off_str} {hex_part} | {ascii_part} ]")
                
                offset += 16
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Custom Hex Dump Utility")
    parser.add_argument("file", help="Path to the file to inspect")
    
    args = parser.parse_args()
    format_hex_dump(args.file)
