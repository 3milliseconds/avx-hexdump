## Project Goal
- Build an implementation that matches the exact observable output of `verify.py`.
- The required row format is:
  - `[0xABSOLUTE_ADDRESS][RELATIVE_INDEX: hex1 hex2 hex3 ... | ascii ]`
- Input is file-based again: the program should accept a file path argument, and `verify.py` is the oracle for that file.

## Canonical Reference
- Treat `verify.py` as the output oracle.
- match the exact output of `verify.py`

## Correctness Requirement
- The implementation is only acceptable if this command passes:
  - `./check.sh`

## Verification
- Always compare the program output against `verify.py`.
- Use `check.sh` as the default verification entry point.

## Performance and Codegen
- Compile optimized builds with AVX enabled, for example `-O3 -mavx2`.
- The AVX ratio for `hexdump_avx` should still be measured and reported, but correctness against `verify.py` comes first.

## Working Rules
- `verify.py` is the oracle definition for expected output.
- Commit and push whenever a new max AVX ratio is obtained.
