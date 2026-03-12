#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_ADDR 0x90E630F8E0ULL
#define FULL_HEX_WIDTH 47
#define PREFIX_LEN 39
#define FULL_LINE_LEN 108

const unsigned char HEX_DIGITS[32] __attribute__((used, aligned(32))) =
    "0123456789abcdef0123456789abcdef";
const unsigned char NIBBLE_MASK_VEC[32] __attribute__((used, aligned(32))) = {
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
};
const unsigned char SIGNED_BIAS_VEC[32] __attribute__((used, aligned(32))) = {
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
};
const unsigned char PRINTABLE_LO_VEC[32] __attribute__((used, aligned(32))) = {
    0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f,
    0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f,
    0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f,
    0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f,
};
const unsigned char DOTS_VEC[32] __attribute__((used, aligned(32))) = {
    '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.',
};
const unsigned char SPACES_VEC[32] __attribute__((used, aligned(32))) = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
};
const unsigned char EXPAND_MASK_VEC[32] __attribute__((used, aligned(32))) = {
    0x00, 0x01, 0x08, 0x02, 0x03, 0x08, 0x04, 0x05,
    0x08, 0x06, 0x07, 0x08, 0x80, 0x80, 0x80, 0x80,
    0x00, 0x01, 0x08, 0x02, 0x03, 0x08, 0x04, 0x05,
    0x08, 0x06, 0x07, 0x08, 0x80, 0x80, 0x80, 0x80,
};

__attribute__((used, noinline)) void write_full_line(
    size_t offset, const char hex_pairs[32], const char ascii[16])
{
    char line[FULL_LINE_LEN];
    int prefix = snprintf(line, sizeof(line), "[0x%016llX][%016llX: ",
                          (unsigned long long)(BASE_ADDR + offset),
                          (unsigned long long)offset);
    char *cursor = line + prefix;
    char *hex = cursor;

    for (size_t i = 0; i < 16; ++i) {
        cursor[0] = hex_pairs[i * 2];
        cursor[1] = hex_pairs[i * 2 + 1];
        cursor += 2;
        if (i != 15) {
            *cursor++ = ' ';
        }
    }

    if ((size_t)(cursor - hex) < FULL_HEX_WIDTH) {
        memset(cursor, ' ', FULL_HEX_WIDTH - (size_t)(cursor - hex));
        cursor = hex + FULL_HEX_WIDTH;
    }

    memcpy(cursor, " | ", 3);
    cursor += 3;
    memcpy(cursor, ascii, 16);
    cursor += 16;
    memcpy(cursor, " ]\n", 3);
    cursor += 3;

    fwrite(line, 1, (size_t)(cursor - line), stdout);
}

__attribute__((used, noinline)) void write_partial_line(
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

void hexdump_avx(const unsigned char *data, size_t len);

__asm__(
    ".intel_syntax noprefix\n"
    ".text\n"
    ".globl hexdump_avx\n"
    ".type hexdump_avx, @function\n"
    "hexdump_avx:\n"
    "    lea rax, [rip + hexdump_avx_consts + 0x50]\n"
    "    vmovdqa xmm0, XMMWORD PTR [rax - 0x50]\n"
    "    vinserti128 ymm0, ymm0, XMMWORD PTR [rax - 0x40], 0x1\n"
    "    vmovdqa xmm1, XMMWORD PTR [rax - 0x30]\n"
    "    vinserti128 ymm1, ymm1, XMMWORD PTR [rax - 0x20], 0x1\n"
    "    vmovdqa xmm2, XMMWORD PTR [rax - 0x10]\n"
    "    vinserti128 ymm2, ymm2, XMMWORD PTR [rax + 0x00], 0x1\n"
    "    vmovdqa xmm3, XMMWORD PTR [rax + 0x10]\n"
    "    vinserti128 ymm3, ymm3, XMMWORD PTR [rax + 0x20], 0x1\n"
    "    vmovdqa xmm4, XMMWORD PTR [rax + 0x30]\n"
    "    vinserti128 ymm4, ymm4, XMMWORD PTR [rax + 0x40], 0x1\n"
    "    vpxor ymm8, ymm8, ymm8\n"
    "    vpcmpeqd ymm9, ymm9, ymm9\n"
    ".Lhexdump_avx_end:\n"
    ".size hexdump_avx, .Lhexdump_avx_end - hexdump_avx\n"
    "hexdump_avx_body:\n"
    "    test rsi, rsi\n"
    "    je .Ldone\n"
    "    push rbx\n"
    "    push r12\n"
    "    push r13\n"
    "    sub rsp, 288\n"
    "    mov r12, rdi\n"
    "    mov r13, rsi\n"
    "    xor ebx, ebx\n"
    "    vmovdqu YMMWORD PTR [rsp + 0], ymm0\n"
    "    vmovdqu YMMWORD PTR [rsp + 32], ymm1\n"
    "    vmovdqu YMMWORD PTR [rsp + 64], ymm2\n"
    "    vmovdqu YMMWORD PTR [rsp + 96], ymm3\n"
    "    vmovdqu YMMWORD PTR [rsp + 128], ymm4\n"
    "    vmovdqu YMMWORD PTR [rsp + 160], ymm8\n"
    "    vmovdqu YMMWORD PTR [rsp + 192], ymm9\n"
    ".Lloop:\n"
    "    mov rax, r13\n"
    "    sub rax, rbx\n"
    "    cmp rax, 16\n"
    "    jb .Ltail\n"
    "    vmovdqu ymm0, YMMWORD PTR [rsp + 0]\n"
    "    vmovdqu ymm1, YMMWORD PTR [rsp + 32]\n"
    "    vmovdqu ymm2, YMMWORD PTR [rsp + 64]\n"
    "    vmovdqu ymm3, YMMWORD PTR [rsp + 96]\n"
    "    vmovdqu ymm4, YMMWORD PTR [rsp + 128]\n"
    "    vmovdqu ymm9, YMMWORD PTR [rsp + 192]\n"
    "    vbroadcasti128 ymm10, XMMWORD PTR [r12 + rbx]\n"
    "    vpsrlw ymm11, ymm10, 4\n"
    "    vpand ymm12, ymm10, ymm1\n"
    "    vpand ymm11, ymm11, ymm1\n"
    "    vpshufb ymm13, ymm0, ymm11\n"
    "    vpshufb ymm12, ymm0, ymm12\n"
    "    vpunpcklbw ymm14, ymm13, ymm12\n"
    "    vpunpckhbw ymm15, ymm13, ymm12\n"
    "    vmovdqu XMMWORD PTR [rsp + 224], xmm14\n"
    "    vmovdqu XMMWORD PTR [rsp + 240], xmm15\n"
    "    vpxor ymm11, ymm10, ymm2\n"
    "    vpcmpgtb ymm12, ymm11, ymm3\n"
    "    vpcmpgtb ymm13, ymm9, ymm11\n"
    "    vpand ymm12, ymm12, ymm13\n"
    "    vpblendvb ymm10, ymm4, ymm10, ymm12\n"
    "    vmovdqu XMMWORD PTR [rsp + 256], xmm10\n"
    "    vzeroupper\n"
    "    mov rdi, rbx\n"
    "    lea rsi, [rsp + 224]\n"
    "    lea rdx, [rsp + 256]\n"
    "    call write_full_line\n"
    "    add rbx, 16\n"
    "    jmp .Lloop\n"
    ".Ltail:\n"
    "    cmp rbx, r13\n"
    "    jae .Lcleanup\n"
    "    mov rdi, r12\n"
    "    add rdi, rbx\n"
    "    mov rsi, rbx\n"
    "    mov rdx, r13\n"
    "    sub rdx, rbx\n"
    "    vzeroupper\n"
    "    call write_partial_line\n"
    ".Lcleanup:\n"
    "    add rsp, 288\n"
    "    pop r13\n"
    "    pop r12\n"
    "    pop rbx\n"
    ".Ldone:\n"
    "    ret\n"
    ".section .rodata\n"
    ".p2align 4\n"
    "hexdump_avx_consts:\n"
    "    .ascii \"0123456789abcdef\"\n"
    "    .ascii \"0123456789abcdef\"\n"
    "    .zero 16, 0x0f\n"
    "    .zero 16, 0x0f\n"
    "    .zero 16, 0x80\n"
    "    .zero 16, 0x80\n"
    "    .zero 16, 0x9f\n"
    "    .zero 16, 0x9f\n"
    "    .zero 16, 0x2e\n"
    "    .zero 16, 0x2e\n"
    ".att_syntax prefix\n");

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
