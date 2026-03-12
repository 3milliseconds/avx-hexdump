#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BASE_ADDR 0x90E630F8E0ULL

void hexdump_avx(const unsigned char *data, size_t len)
{
    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > 16)
            chunk = 16;

        /* [0xABSOLUTE_ADDR][RELATIVE_OFFSET: */
        printf("[0x%016llX][%016llX:",
               (unsigned long long)(BASE_ADDR + offset),
               (unsigned long long)offset);

        /* hex bytes, space-separated, padded to 47 chars for short lines */
        printf(" ");
        for (size_t i = 0; i < chunk; i++) {
            if (i > 0)
                printf(" ");
            printf("%02x", data[offset + i]);
        }
        if (chunk < 16) {
            int cur = (int)(chunk * 3 - 1);
            for (int i = cur; i < 47; i++)
                putchar(' ');
        }

        /* ASCII sidebar */
        printf(" | ");
        for (size_t i = 0; i < chunk; i++) {
            unsigned char c = data[offset + i];
            putchar((c >= 32 && c <= 126) ? c : '.');
        }
        printf(" ]\n");

        offset += 16;
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

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(f);
        return 0;
    }

    unsigned char *data = malloc(file_size);
    if (!data) {
        fprintf(stderr, "Out of memory\n");
        fclose(f);
        return 1;
    }

    fread(data, 1, file_size, f);
    fclose(f);

    hexdump_avx(data, file_size);

    free(data);
    return 0;
}
