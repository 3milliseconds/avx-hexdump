#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void hexdump_avx(const unsigned char *data, size_t len);

__asm__(".include \"hexdump_avx.S\"");

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
