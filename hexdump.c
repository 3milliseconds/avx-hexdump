#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char HEX_LUT[32] __attribute__((aligned(32))) =
    "0123456789abcdef0123456789abcdef";

static __attribute__((always_inline)) inline void encode16_avx2(
    const uint8_t *src, char *dst)
{
    const __m256i bytes = _mm256_broadcastsi128_si256(
        _mm_loadu_si128((const __m128i *)src));
    const __m256i mask4 = _mm256_set1_epi8(0x0f);
    const __m256i lut = _mm256_load_si256((const __m256i *)HEX_LUT);
    const __m256i hi = _mm256_and_si256(_mm256_srli_epi16(bytes, 4), mask4);
    const __m256i lo = _mm256_and_si256(bytes, mask4);
    const __m256i hi_c = _mm256_shuffle_epi8(lut, hi);
    const __m256i lo_c = _mm256_shuffle_epi8(lut, lo);
    const __m256i packed = _mm256_permute2x128_si256(
        _mm256_unpacklo_epi8(hi_c, lo_c),
        _mm256_unpackhi_epi8(hi_c, lo_c),
        0x20);

    _mm256_storeu_si256((__m256i *)dst, packed);
}

static __attribute__((noinline)) size_t encode_tail_hex(
    const uint8_t *src, size_t len, char *dst)
{
    static const char digits[] = "0123456789abcdef";

    for (size_t i = 0; i < len; ++i) {
        dst[i * 2 + 0] = digits[src[i] >> 4];
        dst[i * 2 + 1] = digits[src[i] & 0x0f];
    }

    return len * 2;
}

void hexdump_avx(const uint8_t *data, char *dst)
{
    encode16_avx2(data + 0x00, dst + 0x000);
    encode16_avx2(data + 0x10, dst + 0x020);
    encode16_avx2(data + 0x20, dst + 0x040);
    encode16_avx2(data + 0x30, dst + 0x060);
    encode16_avx2(data + 0x40, dst + 0x080);
    encode16_avx2(data + 0x50, dst + 0x0a0);
    encode16_avx2(data + 0x60, dst + 0x0c0);
    encode16_avx2(data + 0x70, dst + 0x0e0);
    encode16_avx2(data + 0x80, dst + 0x100);
    encode16_avx2(data + 0x90, dst + 0x120);
    encode16_avx2(data + 0xA0, dst + 0x140);
    encode16_avx2(data + 0xB0, dst + 0x160);
    encode16_avx2(data + 0xC0, dst + 0x180);
    encode16_avx2(data + 0xD0, dst + 0x1a0);
    encode16_avx2(data + 0xE0, dst + 0x1c0);
    encode16_avx2(data + 0xF0, dst + 0x1e0);
}

static int read_all_stdin(uint8_t **data_out, size_t *len_out)
{
    uint8_t *data = NULL;
    size_t len = 0;
    size_t cap = 0;

    for (;;) {
        uint8_t chunk[4096];
        ssize_t n = read(STDIN_FILENO, chunk, sizeof(chunk));

        if (n < 0) {
            free(data);
            return -1;
        }
        if (n == 0) {
            break;
        }

        if (len + (size_t)n > cap) {
            size_t new_cap = cap ? cap : 4096;
            while (new_cap < len + (size_t)n) {
                new_cap *= 2;
            }

            uint8_t *new_data = realloc(data, new_cap);
            if (!new_data) {
                free(data);
                return -1;
            }

            data = new_data;
            cap = new_cap;
        }

        memcpy(data + len, chunk, (size_t)n);
        len += (size_t)n;
    }

    *data_out = data;
    *len_out = len;
    return 0;
}

static int write_all_stdout(const char *buf, size_t len)
{
    size_t written = 0;

    while (written < len) {
        ssize_t n = write(STDOUT_FILENO, buf + written, len - written);
        if (n <= 0) {
            return -1;
        }
        written += (size_t)n;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    uint8_t *input = NULL;
    size_t input_len = 0;
    char *output = NULL;
    size_t output_len;
    size_t avx_blocks;
    size_t avx_len;
    size_t tail_len;

    if (argc != 1) {
        fprintf(stderr, "Usage: %s < input.bin\n", argv[0]);
        return 1;
    }

    if (read_all_stdin(&input, &input_len) != 0) {
        fprintf(stderr, "Error: failed to read stdin\n");
        free(input);
        return 1;
    }

    if (input_len == 0) {
        free(input);
        return 0;
    }

    output = malloc(input_len * 2 + 1);
    if (!output) {
        fprintf(stderr, "Out of memory\n");
        free(input);
        return 1;
    }

    avx_blocks = input_len / 256;
    avx_len = avx_blocks * 256;
    tail_len = input_len - avx_len;

    output_len = 0;
    for (size_t i = 0; i < avx_blocks; ++i) {
        hexdump_avx(input + i * 256, output + output_len);
        output_len += 512;
    }
    if (tail_len != 0) {
        output_len += encode_tail_hex(input + avx_len, tail_len, output + output_len);
    }
    output[output_len++] = '\n';

    if (write_all_stdout(output, output_len) != 0) {
        fprintf(stderr, "Error: failed to write stdout\n");
        free(output);
        free(input);
        return 1;
    }

    free(output);
    free(input);
    return 0;
}
