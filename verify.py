import sys
import argparse


def format_hex_dump(stream, base_addr):
    offset = 0
    while True:
        chunk = stream.read(16)
        if not chunk:
            break

        abs_addr_str = f"[0x{base_addr + offset:016X}]"
        rel_off_str = f"[{offset:016X}:"
        hex_part = " ".join(f"{b:02x}" for b in chunk)
        ascii_part = "".join(chr(b) if 32 <= b <= 126 else "." for b in chunk)

        print(f"{abs_addr_str}{rel_off_str} {hex_part} | {ascii_part} ]")
        offset += len(chunk)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Custom Hex Dump Utility")
    parser.add_argument(
        "base_addr",
        help="Initial absolute address to print, for example 0x90E630F8E0",
    )

    args = parser.parse_args()
    format_hex_dump(sys.stdin.buffer, int(args.base_addr, 0))
