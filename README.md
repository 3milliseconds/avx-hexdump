# avx-hexdump

Trying to do some low-level SIMD / AVX performance engineering without prior AVX experience.

The goal is to produce a hexdump implementation that matches the Python oracle exactly while pushing most of the hot path into AVX instructions.
