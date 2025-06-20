#include "encode.h"
#include <zstd.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <stdio.h>

int encode(const char* input, int input_len, char* output) {
    size_t max_compressed = ZSTD_compressBound(input_len);
    size_t compressed_size = ZSTD_compress(output, max_compressed, input, input_len, 1); // 圧縮レベル1
    if (ZSTD_isError(compressed_size)) {
        fprintf(stderr, "ZSTD_compress error: %s\n", ZSTD_getErrorName(compressed_size));
        return 0;
    }
    return (int)compressed_size;
}

int decode(const char* input, int input_len, char* output) {
    // 出力バッファは元のPCMサイズ分確保されている前提
    size_t decompressed_size = ZSTD_decompress(output, 1024, input, input_len);
    if (ZSTD_isError(decompressed_size)) {
        fprintf(stderr, "ZSTD_decompress error: %s\n", ZSTD_getErrorName(decompressed_size));
        return 0;
    }
    return (int)decompressed_size;
}

