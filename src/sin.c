#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @brief 正弦波のサンプリング値（バイナリ）を標準出力に出力する
 * argv[1]: 振幅
 * argv[2]: 正弦波周波数
 * argv[3]: プロット数
 * サンプリング周波数は44100Hz固定
 * @example ./build/sin 10000 440 88200 | play -t raw -b 16 -c 1 -e s -r 44100 -
 * @example ./build/sin 10000 440 88200 > ./data/sin.raw
 */
int main(int argc, char *argv[]) {
  double sampling_rate = 44100;
  double A = atoi(argv[1]);
  double f = atoi(argv[2]);
  int n = atoi(argv[3]);

  for (int i = 0; i < n; i++) {
    double t = (double)i / sampling_rate;
    double th = 2 * M_PI * f * t;
    int16_t val = A * sin(th);
    write(STDOUT_FILENO, &val, sizeof(val));
  }
}