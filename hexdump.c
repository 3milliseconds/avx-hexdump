#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const uint64_t BASE_ADDR = 0x00000090E630F8E0ULL;

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

void hexdump_avx(const uint8_t *data, char *dst)
{
    encode16_avx2(data, dst);
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

static void append_u64_hex_upper(char **dst, uint64_t value)
{
    static const char digits[] = "0123456789ABCDEF";

    for (int shift = 60; shift >= 0; shift -= 4) {
        *(*dst)++ = digits[(value >> shift) & 0x0f];
    }
}

static void append_byte_hex_lower(char **dst, uint8_t value)
{
    static const char digits[] = "0123456789abcdef";

    *(*dst)++ = digits[value >> 4];
    *(*dst)++ = digits[value & 0x0f];
}

static int dump_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    uint8_t chunk[16];
    size_t offset = 0;

    if (!f) {
        fprintf(stderr, "Error: failed to open '%s'\n", path);
        return 1;
    }

    for (;;) {
        size_t n = fread(chunk, 1, sizeof(chunk), f);

        if (n == 0) {
            if (ferror(f)) {
                fprintf(stderr, "Error: failed to read '%s'\n", path);
                fclose(f);
                return 1;
            }
            break;
        }

        {
            char line[128];
            char full_hex[32];
            char *p = line;

            *p++ = '[';
            *p++ = '0';
            *p++ = 'x';
            append_u64_hex_upper(&p, BASE_ADDR + offset);
            *p++ = ']';
            *p++ = '[';
            append_u64_hex_upper(&p, (uint64_t)offset);
            *p++ = ':';
            *p++ = ' ';

            if (n == sizeof(chunk)) {
                hexdump_avx(chunk, full_hex);
                for (size_t i = 0; i < n; ++i) {
                    *p++ = full_hex[i * 2];
                    *p++ = full_hex[i * 2 + 1];
                    if (i + 1 != n) {
                        *p++ = ' ';
                    }
                }
            } else {
                for (size_t i = 0; i < n; ++i) {
                    append_byte_hex_lower(&p, chunk[i]);
                    if (i + 1 != n) {
                        *p++ = ' ';
                    }
                }
            }

            *p++ = ' ';
            *p++ = '|';
            *p++ = ' ';

            for (size_t i = 0; i < n; ++i) {
                unsigned char c = chunk[i];
                *p++ = (c >= 32 && c <= 126) ? (char)c : '.';
            }

            *p++ = ' ';
            *p++ = ']';
            *p++ = '\n';

            if (write_all_stdout(line, (size_t)(p - line)) != 0) {
                fprintf(stderr, "Error: failed to write stdout\n");
                fclose(f);
                return 1;
            }
        }

        offset += n;
    }

    fclose(f);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    return dump_file(argv[1]);
}
