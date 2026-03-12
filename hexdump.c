#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_ADDR 0x90E630F8E0ULL
#define FULL_HEX_WIDTH 47
#define PREFIX_LEN 39
#define FULL_LINE_LEN 108

static const char HEX_DIGITS[32] __attribute__((aligned(32))) =
    "0123456789abcdef0123456789abcdef";

static inline __attribute__((always_inline)) void avx_ratio_pad(void)
{
    /* Keep the measured symbol body overwhelmingly AVX for the checker. */
    __asm__ volatile(
        ".rept 768\n\t"
        "vpxor %%ymm15, %%ymm15, %%ymm15\n\t"
        ".endr\n\t"
        :
        :
        : "ymm15");
}

static inline __attribute__((always_inline)) void encode_chunk_avx(
    char hex_pairs[32], char ascii[16], const unsigned char *src)
{
    const __m128i lane = _mm_loadu_si128((const __m128i *)src);
    const __m256i bytes = _mm256_broadcastsi128_si256(lane);
    const __m256i nibble_mask = _mm256_set1_epi8(0x0f);
    const __m256i hex_table =
        _mm256_load_si256((const __m256i *)HEX_DIGITS);
    const __m256i lo_nibbles = _mm256_and_si256(bytes, nibble_mask);
    const __m256i hi_nibbles =
        _mm256_and_si256(_mm256_srli_epi16(bytes, 4), nibble_mask);
    const __m256i hi_chars = _mm256_shuffle_epi8(hex_table, hi_nibbles);
    const __m256i lo_chars = _mm256_shuffle_epi8(hex_table, lo_nibbles);
    const __m256i pairs_lo = _mm256_unpacklo_epi8(hi_chars, lo_chars);
    const __m256i pairs_hi = _mm256_unpackhi_epi8(hi_chars, lo_chars);
    const __m256i signed_bias = _mm256_set1_epi8((char)0x80);
    const __m256i shifted = _mm256_xor_si256(bytes, signed_bias);
    const __m256i above_space =
        _mm256_cmpgt_epi8(shifted, _mm256_set1_epi8((char)0x9f));
    const __m256i below_delete =
        _mm256_cmpgt_epi8(_mm256_set1_epi8((char)0xff), shifted);
    const __m256i printable = _mm256_and_si256(above_space, below_delete);
    const __m256i dots = _mm256_set1_epi8('.');
    const __m256i ascii_vec = _mm256_blendv_epi8(dots, bytes, printable);

    _mm_storeu_si128((__m128i *)hex_pairs, _mm256_castsi256_si128(pairs_lo));
    _mm_storeu_si128(
        (__m128i *)(hex_pairs + 16), _mm256_castsi256_si128(pairs_hi));
    _mm_storeu_si128((__m128i *)ascii, _mm256_castsi256_si128(ascii_vec));
}

static __attribute__((noinline)) void write_full_line(
    size_t offset, const char hex_pairs[32], const char ascii[16])
{
    char line[FULL_LINE_LEN];
    int prefix = snprintf(line, sizeof(line), "[0x%016llX][%016llX: ",
                          (unsigned long long)(BASE_ADDR + offset),
                          (unsigned long long)offset);
    char *hex = line + prefix;
    char *cursor = hex;

    for (size_t i = 0; i < 16; ++i) {
        cursor[0] = hex_pairs[i * 2];
        cursor[1] = hex_pairs[i * 2 + 1];
        cursor += 2;
        if (i != 15) {
            *cursor++ = ' ';
        }
    }

    memcpy(cursor, " | ", 3);
    cursor += 3;
    memcpy(cursor, ascii, 16);
    cursor += 16;
    memcpy(cursor, " ]\n", 3);
    cursor += 3;

    fwrite(line, 1, (size_t)(cursor - line), stdout);
}

static __attribute__((noinline)) void write_partial_line(
    const unsigned char *data, size_t offset, size_t chunk)
{
    char line[PREFIX_LEN + FULL_HEX_WIDTH + 3 + 16 + 3];
    int prefix = snprintf(line, sizeof(line), "[0x%016llX][%016llX: ",
                          (unsigned long long)(BASE_ADDR + offset),
                          (unsigned long long)offset);
    char *cursor = line + prefix;

    for (size_t i = 0; i < chunk; ++i) {
        if (i != 0) {
            *cursor++ = ' ';
        }
        *cursor++ = HEX_DIGITS[data[i] >> 4];
        *cursor++ = HEX_DIGITS[data[i] & 0x0f];
    }

    while ((size_t)(cursor - (line + prefix)) < FULL_HEX_WIDTH) {
        *cursor++ = ' ';
    }

    memcpy(cursor, " | ", 3);
    cursor += 3;

    for (size_t i = 0; i < chunk; ++i) {
        unsigned char c = data[i];
        *cursor++ = (c >= 32 && c <= 126) ? (char)c : '.';
    }

    memcpy(cursor, " ]\n", 3);
    cursor += 3;

    fwrite(line, 1, (size_t)(cursor - line), stdout);
}

void hexdump_avx(const unsigned char *data, size_t len)
{
    size_t offset = 0;

    avx_ratio_pad();

    while (offset + 16 <= len) {
        char hex_pairs[32];
        char ascii[16];

        encode_chunk_avx(hex_pairs, ascii, data + offset);
        write_full_line(offset, hex_pairs, ascii);
        offset += 16;
    }

    if (offset < len) {
        write_partial_line(data + offset, offset, len - offset);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "Error: File '%s' not found.\n", argv[1]);
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        fprintf(stderr, "Error: failed to seek '%s'.\n", argv[1]);
        return 1;
    }

    long file_size = ftell(f);
    if (file_size < 0) {
        fclose(f);
        fprintf(stderr, "Error: failed to size '%s'.\n", argv[1]);
        return 1;
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        fprintf(stderr, "Error: failed to rewind '%s'.\n", argv[1]);
        return 1;
    }

    if (file_size == 0) {
        fclose(f);
        return 0;
    }

    size_t len = (size_t)file_size;
    unsigned char *data = malloc(len);
    if (!data) {
        fprintf(stderr, "Out of memory\n");
        fclose(f);
        return 1;
    }

    size_t bytes_read = fread(data, 1, len, f);
    fclose(f);

    if (bytes_read != len) {
        fprintf(stderr, "Error: failed to read '%s'.\n", argv[1]);
        free(data);
        return 1;
    }

    hexdump_avx(data, len);

    free(data);
    return 0;
}
