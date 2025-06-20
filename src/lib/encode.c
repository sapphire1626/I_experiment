#include "encode.h"
#include <zstd.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <stdio.h>

#define NOISE_EST_FRAMES 10
#define ENCODE_BUF_SIZE 1024

// FFT/IFFT用関数
void fft(complex double* x, complex double* y, int n) {
    if (n == 1) {
        y[0] = x[0];
        return;
    }
    int m = n / 2;
    complex double even[m], odd[m], Y_even[m], Y_odd[m];
    for (int i = 0; i < m; i++) {
        even[i] = x[2 * i];
        odd[i] = x[2 * i + 1];
    }
    fft(even, Y_even, m);
    fft(odd, Y_odd, m);
    for (int k = 0; k < m; k++) {
        complex double t = cexp(-2.0 * I * M_PI * k / n) * Y_odd[k];
        y[k] = Y_even[k] + t;
        y[k + m] = Y_even[k] - t;
    }
}

void ifft(complex double* y, complex double* x, int n) {
    for (int i = 0; i < n; i++) y[i] = conj(y[i]);
    fft(y, x, n);
    for (int i = 0; i < n; i++) {
        x[i] = conj(x[i]) / n;
    }
}

// スペクトル減算用ノイズ推定
static float noise_mag[ENCODE_BUF_SIZE/2] = {0};
static int noise_est_count = 0;

void spectral_subtraction(complex double* Y, int n) {
    for (int k = 0; k < n; k++) {
        float mag = cabs(Y[k]);
        float phase = carg(Y[k]);
        float sub = mag - noise_mag[k];
        if (sub < 0) sub = 0;
        Y[k] = sub * cexp(I * phase);
    }
}

void bandpass_and_spectral_sub(short* data, int len) {
    int n = len;
    complex double x[n], y[n];
    for (int i = 0; i < n; i++) x[i] = data[i];
    fft(x, y, n);
    // バンドパス（300Hz～3400Hz）
    double df = 44100.0 / n;
    int k_low = (int)(300 / df + 0.5);
    int k_high = (int)(3400 / df + 0.5);
    for (int k = 0; k < n; k++) {
        int k_sym = (k <= n/2) ? k : n - k;
        if (k_sym < k_low || k_sym > k_high) y[k] = 0;
    }
    // ノイズ推定
    if (noise_est_count < NOISE_EST_FRAMES) {
        for (int k = 0; k < n; k++) {
            noise_mag[k] += cabs(y[k]) / NOISE_EST_FRAMES;
        }
        noise_est_count++;
    } else {
        spectral_subtraction(y, n);
    }
    ifft(y, x, n);
    for (int i = 0; i < n; i++) {
        double v = creal(x[i]);
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        data[i] = (short)v;
    }
}

int encode(const char* input, int input_len, char* output) {
    // 音声前処理: FFT+バンドパス+スペクトル減算
    bandpass_and_spectral_sub((short*)input, input_len / 2);
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

