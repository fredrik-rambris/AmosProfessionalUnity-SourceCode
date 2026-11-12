#include <stdint.h>

#include <stdint.h>
#include <stdlib.h>

#include "common.h"
#include <arpa/inet.h>

int main(int argc, char **argv)
{
    FILE *out;
    uint8_t *chunk = NULL;
    size_t size;
    uint32_t chunk_size;

    if (argc != 3)
    {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    printf("Loading %s...\n", argv[1]);
    chunk = (uint8_t *)read_file(argv[1], &size);

    open_output_file(&out, argv[2], NULL);

    chunk_size = *(uint32_t *)(chunk+28);
    chunk_size = ntohl(chunk_size)*4;

    printf("Saving %s (%d bytes)...\n", argv[2], chunk_size);
    fwrite(chunk+32, 1, chunk_size, out);

    if (chunk)
    {
        free(chunk);
    }
    return 0;
}
