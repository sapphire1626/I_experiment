#include "encode.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

/* 高速(逆)フーリエ変換;
   w は1のn乗根.
   フーリエ変換の場合   偏角 -2 pi / n
   逆フーリエ変換の場合 偏角  2 pi / n
   xが入力でyが出力.
   xも破壊される
 */
void fft_r(complex double* x, complex double* y, long n, complex double w) {
  if (n == 1) {
    y[0] = x[0];
  } else {
    complex double W = 1.0;
    long i;
    for (i = 0; i < n / 2; i++) {
      y[i] = (x[i] + x[i + n / 2]);             /* 偶数行 */
      y[i + n / 2] = W * (x[i] - x[i + n / 2]); /* 奇数行 */
      W *= w;
    }
    fft_r(y, x, n / 2, w * w);
    fft_r(y + n / 2, x + n / 2, n / 2, w * w);
    for (i = 0; i < n / 2; i++) {
      y[2 * i] = x[i];
      y[2 * i + 1] = x[i + n / 2];
    }
  }
}

void fft(complex double* x, complex double* y, long n) {
  long i;
  double arg = 2.0 * M_PI / n;
  complex double w = cos(arg) - 1.0j * sin(arg);
  fft_r(x, y, n, w);
  for (i = 0; i < n; i++) y[i] /= n;
}

void ifft(complex double* y, complex double* x, long n) {
  double arg = 2.0 * M_PI / n;
  complex double w = cos(arg) + 1.0j * sin(arg);
  fft_r(y, x, n, w);
}

int encode(const char* input, int input_len, char* output) {
  int nsamples = input_len / 2;
  short *buf = (short*)input;
  complex double* X = malloc(nsamples * sizeof(complex double));
  complex double* Y = malloc(nsamples * sizeof(complex double));
  for (int i = 0; i < nsamples; i++) {
    X[i] = buf[i];
  }

  fft(X, Y, nsamples);

  double df = (double)SAMPLE_RATE / nsamples;
  int k_low = (int)(MIN_FREQ / df + 0.5);  // min_freq成分のインデックス
  int k_high = (int)(MAX_FREQ / df + 0.5); // max_freq成分のインデックス

  ((int16_t*)output)[0] = nsamples; // ifftで必要

  int count = 0;
  for (int k = 0; k < nsamples; k++) {
    int k_sym = (k <= nsamples / 2) ? k : nsamples - k;
    if (k_sym >= k_low && k_sym <= k_high) {
      double re = creal(Y[k]);
      double im = cimag(Y[k]);
      ((int16_t*)output)[3 * count + 1] = k;
      ((int16_t*)output)[3 * count + 2] = (int16_t)(re);
      ((int16_t*)output)[3 * count + 3] = (int16_t)(im);
      count++;
    }
  }

  free(X);
  free(Y);
  return count * 3 * sizeof(int16_t);
}

int decode(const char* input, int input_len, char* output) {
  int nsamples = ((const int16_t*)input)[0]; // ifftで必要
  int nindex = (input_len / sizeof(int16_t) - 1) / 3;

  complex double* Y = malloc(nsamples * sizeof(complex double));
  complex double* X = malloc(nsamples * sizeof(complex double));

  for (int i = 0; i < nindex; i++) {
    int idx = ((const int16_t*)input)[3 * i + 1];
    int16_t re = ((const int16_t*)input)[3 * i + 2];
    int16_t im = ((const int16_t*)input)[3 * i + 3];
    Y[idx] = re + im * I;
  }

  ifft(Y, X, nsamples);

  for (int i = 0; i < nsamples; i++) {
    double val = creal(X[i]) / nsamples;
    ((short*)output)[i] = (short)val;
  }

  free(X);
  free(Y);
  return nsamples * sizeof(short);
}