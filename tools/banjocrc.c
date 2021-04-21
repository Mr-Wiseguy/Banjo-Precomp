#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void print_usage(const char *name)
{
    printf("Usage: %s [input_file]\n", name);
    exit(EXIT_SUCCESS);
}

void print_failed_to_open(const char *filename)
{
    fprintf(stderr, "Failed to open file: %s\n", filename);
    exit(EXIT_FAILURE);
}

void print_file_read_error(const char *filename)
{
    fprintf(stderr, "Error reading file: %s\n", filename);
    exit(EXIT_FAILURE);
}

void calculate_crc(FILE *input, uint32_t *crc1out, uint32_t *crc2out, const char *filename)
{
    uint32_t crc1 = 0;
    uint32_t crc2 = UINT32_MAX;
    uint8_t *fileBuffer;
    uint8_t *curBytePtr;
    size_t fileLen;

    fseek(input, 0, SEEK_END);
    fileLen = ftell(input);
    fseek(input, 0, SEEK_SET);

    fileBuffer = malloc(fileLen);
    if (fread(fileBuffer, 1, fileLen, input) < fileLen)
    {
        free(fileBuffer);
        print_file_read_error(filename);
    }

    curBytePtr = fileBuffer;
    while (fileLen)
    {
        uint8_t curByte = *curBytePtr;
        crc1 += curByte;
        crc2 ^= curByte << (crc1 & 0x17);
        curBytePtr++;
        fileLen--;
    }

    *crc1out = crc1;
    *crc2out = crc2;
    
    free(fileBuffer);
}

int main(int argc, const char *const argv[])
{
    int i;

    if (argc < 2)
    {
        print_usage(argv[0]);
    }

    for (i = 1; i < argc; i++)
    {
        FILE *inputFile;
        uint32_t crc1, crc2;

        inputFile = fopen(argv[i], "rb");

        if (inputFile == NULL)
        {
            print_failed_to_open(argv[i]);
        }

        calculate_crc(inputFile, &crc1, &crc2, argv[i]);

        crc1 = __builtin_bswap32(crc1);
        crc2 = __builtin_bswap32(crc2);

        fwrite(&crc1, 1, 4, stdout);
        fwrite(&crc2, 1, 4, stdout);
    }
}
