#include <stdio.h>
#include <stdlib.h>

// 秒数 => プロット個数
int secToN(double sec) {
  return sec * 44100;
}

/**
 * @brief 音階バイナリ値を標準出力に出力する
 * argv[1]: 音階個数
 * @example ./build/doremi 10 | play -t raw -b 16 -c 1 -e s -r 44100 -
 * @example ./build/doremi 10 > ./data/doremi.raw
 */
int main(int argc, char* argv[]) {
  const int n = atoi(argv[1]);

  const int sound_strength = 10000;
  const double sound_duration = 0.3;  // 一音の継続時間 [秒]

  char sound_code[3] = "C4";

  for (int i = 0; i < n; i++) {
    char command[256];
    snprintf(command, sizeof(command), "./build/sin %d %s %d", sound_strength,
             sound_code, secToN(sound_duration));
    int _ = system(command);

    if (sound_code[0] == 'B') {
      sound_code[1]++;
    }

    if (sound_code[0] == 'G') {
      sound_code[0] = 'A';
    } else {
      sound_code[0]++;
    }
  }
}