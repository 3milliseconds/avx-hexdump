#!/usr/bin/env python3

import argparse
import random
from pathlib import Path


EDGE_CASE_SIZES = [0, 1, 2, 7, 8, 15, 16, 17, 31, 32, 33]


def write_case(path: Path, size: int, rng: random.Random) -> None:
    path.write_bytes(bytes(rng.getrandbits(8) for _ in range(size)))


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate small random binary inputs for hexdump tests."
    )
    parser.add_argument(
        "--output-dir",
        default="inputs",
        help="Directory where .bin fixtures will be written.",
    )
    parser.add_argument(
        "--random-count",
        type=int,
        default=12,
        help="Number of additional random-sized fixtures to generate.",
    )
    parser.add_argument(
        "--max-size",
        type=int,
        default=64,
        help="Maximum size in bytes for random-sized fixtures.",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=1337,
        help="Seed for deterministic fixture generation.",
    )
    args = parser.parse_args()

    if args.max_size < 0:
        raise SystemExit("--max-size must be non-negative")
    if args.random_count < 0:
        raise SystemExit("--random-count must be non-negative")

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    rng = random.Random(args.seed)

    for index, size in enumerate(EDGE_CASE_SIZES):
        write_case(output_dir / f"edge_{index:02d}_{size:02d}.bin", size, rng)

    for index in range(args.random_count):
        size = rng.randint(0, args.max_size)
        write_case(output_dir / f"rand_{index:02d}_{size:02d}.bin", size, rng)


if __name__ == "__main__":
    main()
