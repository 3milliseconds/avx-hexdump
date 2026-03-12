## Project Goal
- Build a hexdump implementation that produces byte-for-byte identical output to the reference behavior in `verify.py`.
- The implementation should use AVX instructions for the hot path. Target roughly 90% of the hexdump function body in AVX-capable code, with scalar code reserved for setup, tail handling, and unavoidable formatting glue.

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

## Working Rules
- DONOT MODIFY any verification files
- Commit and push whenever a new max avx value is obtained.