## Project Goal
- Build a hexdump implementation that produces byte-for-byte identical output to the reference behavior in `verify.py`.
- The implementation should use AVX instructions for the hot path.
- Prefer a real assembly implementation for `hexdump_avx` (for example `hexdump_avx.S`) with a small C entry point / driver.
- It is acceptable and expected to add a `Makefile` so `check.sh` builds the assembly implementation via `make`.

## Canonical Reference
- Treat `verify.py` as the output oracle.
- If there is any ambiguity, match the observable output of `verify.py`

## Correctness Requirement
- The implementation is only acceptable if this command passes:
  - `./check.sh`

## Verification
- Always compare the AVX implementation output against the reference output from `verify.py`.
- Use `check.sh` as the default verification entry point once a compiled target exists.
- run `check.sh` to verify for correctness

## Performance and Codegen
- Compile optimized builds with AVX enabled, for example `-O3 -mavx2`, unless the toolchain requires an equivalent flag set.
- If using assembly, keep the real implementation inside the `hexdump_avx` symbol that `check.sh` disassembles.

## Honesty Requirement
- Do not add dead AVX instructions, nop-equivalents, or padding whose only purpose is to inflate the AVX ratio.
- Do not split scalar-heavy work into helper symbols purely to hide it from the checker's measurement scope.
- Do not use symbol-boundary tricks so `hexdump_avx` disassembles as only a setup stub while the real body lives elsewhere.
- The measured AVX ratio must come from the real implementation, not from checker-specific code shaping.

## Working Rules
- DONOT MODIFY any verification files
- Commit and push whenever a new max avx value is obtained.
