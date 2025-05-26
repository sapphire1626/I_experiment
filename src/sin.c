#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

double parseSoundCode(const char* code);

/**
 * @brief 正弦波のサンプリング値（バイナリ）を標準出力に出力する
 * argv[1]: 振幅
 * argv[2]: 正弦波周波数
 * argv[3]: プロット数
 * サンプリング周波数は44100Hz固定
 * @example ./build/sin 10000 440 88200 | play -t raw -b 16 -c 1 -e s -r 44100 -
 * @example ./build/sin 10000 440 88200 > ./data/sin.raw
 */
int main(int argc, char* argv[]) {
  const double sampling_rate = 44100;
  const double A = atoi(argv[1]);
  double f;
  if ('0' <= argv[2][0] && argv[2][0] <= '9') {
    f = atoi(argv[2]);
  } else {
    f = parseSoundCode(argv[2]);
  }
  const int n = atoi(argv[3]);

  for (int i = 0; i < n; i++) {
    double t = (double)i / sampling_rate;
    double th = 2 * M_PI * f * t;
    int16_t val = A * sin(th);
    int _ = write(STDOUT_FILENO, &val, sizeof(val));
  }
}

double parseSoundCode(const char* code) {
  const int code_len = strlen(code);

  if (code_len < 2 || code_len > 3) {
    fprintf(stderr, "Invalid code: %s\n", code);
    return -1;
  }
  if (code_len == 3 && code[1] != '#' && code[1] != 'b') {
    fprintf(stderr, "Invalid code: %s\n", code);
    return -1;
  }
  const int octave_index = code_len == 2 ? 1 : 2;
  if (code[octave_index] < '0' || code[octave_index] > '8') {
    fprintf(stderr, "Invalid code: %s\n", code);
    return -1;
  }

  int stepup;
  switch (code[0]) {
    case 'A':
    case 'a':
      if (code_len == 2) {
        stepup = 0;
      } else if (code[1] == '#') {
        stepup = 1;
      } else {
        stepup = -1;
      }
      break;
    case 'B':
    case 'b':
      if (code_len == 2) {
        stepup = 2;
      } else if (code[1] == '#') {
        fprintf(stderr, "Invalid code: %s\n", code);
      } else {
        stepup = 1;
      }
      break;
    case 'C':
    case 'c':
      stepup = code_len == 2 ? -9 : -8;
      if (code_len == 2) {
        stepup = -9;
      } else if (code[1] == '#') {
        stepup = -8;
      } else {
        fprintf(stderr, "Invalid code: %s\n", code);
        return -1;
      }
      break;
    case 'D':
    case 'd':
      if (code_len == 2) {
        stepup = -7;
      } else if (code[1] == '#') {
        stepup = -6;
      } else {
        stepup = -8;
      }
      break;
    case 'E':
    case 'e':
      if (code_len == 2) {
        stepup = -5;
      } else if (code[1] == '#') {
        fprintf(stderr, "Invalid code: %s\n", code);
        return -1;
      } else {
        stepup = -6;
      }
      break;
    case 'F':
    case 'f':
      if (code_len == 2) {
        stepup = -4;
      } else if (code[1] == '#') {
        stepup = -3;
      } else {
        fprintf(stderr, "Invalid code: %s\n", code);
        return -1;
      }
      break;
    case 'G':
    case 'g':
      if (code_len == 2) {
        stepup = -2;
      } else if (code[1] == '#') {
        stepup = -1;
      } else {
        stepup = -3;
      }
      break;
    default:
      fprintf(stderr, "Invalid code: %s\n", code);
      return -1;
  }

  const int octave = code[octave_index] - '0';
  return 440.0 * pow(2.0, (octave - 4) + stepup / 12.0);
}