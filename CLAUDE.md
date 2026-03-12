## Project Goal
- Maximize the AVX instruction ratio inside `hexdump_avx`.
- It is acceptable to change both input handling and output format to support a more AVX-friendly implementation.
- Prefer a simple stream encoder over a strict hexdump replica if that improves the AVX-heavy hot path.

## Input / Output
- Reading from `stdin` is the preferred interface.
- A simplified output format is acceptable. For example, emitting a continuous lowercase hex stream is fine.
- Exact compatibility with `verify.py` is no longer required for this variant.

## Verification
- The primary verification target is the AVX instruction mix of `hexdump_avx`.
- Measure that with `objdump -d -M intel --disassemble=hexdump_avx ./hexdump`.
- Functional verification only needs to confirm that the chosen simplified format is produced consistently.

## Performance and Codegen
- Compile optimized builds with AVX enabled, for example `-O3 -mavx2`.
- Prefer keeping scalar-heavy setup, allocation, I/O, and tail handling outside the measured hot path when practical.
- Prefer inlining real AVX conversion work into `hexdump_avx`.

## Working Rules
- DONOT MODIFY any verification files
- Commit and push whenever a new max avx value is obtained.
