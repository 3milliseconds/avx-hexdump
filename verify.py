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

                abs_addr_str = f"[0x{base_addr + offset:016X}]"
                rel_off_str = f"[{offset:016X}:"
                hex_part = " ".join(f"{b:02x}" for b in chunk)
                ascii_part = "".join(chr(b) if 32 <= b <= 126 else "." for b in chunk)

                print(f"{abs_addr_str}{rel_off_str} {hex_part} | {ascii_part} ]")
                offset += 16
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Custom Hex Dump Utility")
    parser.add_argument("file", help="Path to the file to inspect")
    
    args = parser.parse_args()
    format_hex_dump(args.file)
