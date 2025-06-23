
#include "encode.h"

#include <fftw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zstd.h>

#define NOISE_EST_FRAMES 10  // テキトーに決めた

static float noise_mag[BUFFER_SIZE / 2] = {0};
static int noise_est_count = 0;

void bandpass_noise_byebye(short* data, int len) {
  int n = len;
  fftw_complex* x = fftw_malloc(sizeof(fftw_complex) * n);
  fftw_complex* y = fftw_malloc(sizeof(fftw_complex) * n);

  for (int i = 0; i < n; i++) {
    x[i][0] = data[i];  // re
    x[i][1] = 0.0;      // im
  }

  fftw_plan plan_fwd = fftw_plan_dft_1d(n, x, y, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_plan plan_inv = fftw_plan_dft_1d(n, y, x, FFTW_BACKWARD, FFTW_ESTIMATE);

  fftw_execute(plan_fwd);

  // 300Hz～3400Hz
  double df = 44100.0 / n;
  int k_low = (int)(300 / df + 0.5);
  int k_high = (int)(3400 / df + 0.5);
  for (int k = 0; k < n; k++) {
    int k_sym = (k <= n / 2) ? k : n - k;
    if (k_sym < k_low || k_sym > k_high) {
      y[k][0] = 0.0;
      y[k][1] = 0.0;
    }
  }

  // ノイズ軽減
  if (noise_est_count < NOISE_EST_FRAMES) {
    for (int k = 0; k < n; k++) {
      double mag = hypot(y[k][0], y[k][1]);
      noise_mag[k] += mag / NOISE_EST_FRAMES;
    }
    noise_est_count++;
  } else {
    for (int k = 0; k < n; k++) {
      double mag = hypot(y[k][0], y[k][1]);
      double phase = atan2(y[k][1], y[k][0]);
      double sub = mag - noise_mag[k];
      if (sub < 0) sub = 0;
      y[k][0] = sub * cos(phase);
      y[k][1] = sub * sin(phase);
    }
  }

  fftw_execute(plan_inv);

  // スケーリング
  for (int i = 0; i < n; i++) {
    double v = x[i][0] / n;
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    data[i] = (short)v;
  }

  fftw_destroy_plan(plan_fwd);
  fftw_destroy_plan(plan_inv);
  fftw_free(x);
  fftw_free(y);
}

int encode(const char* input, int input_len, char* output) {
  // バンドパス+ノイズ軽減+圧縮
  bandpass_noise_byebye((short*)input, input_len / 2);
  size_t max_compressed = ZSTD_compressBound(input_len);
  size_t compressed_size =
      ZSTD_compress(output, max_compressed, input, input_len, 1);
  if (ZSTD_isError(compressed_size)) {
    fprintf(stderr, "ZSTD_compress error: %s\n",
            ZSTD_getErrorName(compressed_size));
    return 0;
  }
  return (int)compressed_size;
}

int decode(const char* input, int input_len, char* output) {
  // 解凍
  size_t decompressed_size = ZSTD_decompress(output, 1024, input, input_len);
  if (ZSTD_isError(decompressed_size)) {
    fprintf(stderr, "ZSTD_decompress error: %s\n",
            ZSTD_getErrorName(decompressed_size));
    return 0;
  }
  return (int)decompressed_size;
}
