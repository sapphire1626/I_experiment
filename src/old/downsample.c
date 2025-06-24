#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/// @brief
/// 標準入力から音声データを読み込み、指定されたインターバルでダウンサンプリングして標準出力に書き出す
/// @param argv[1]: インターバル
/// @example sox ./data/obama.wav -t raw -b 16 -c 1 -e s -r 44100 - |
/// ./build/downsample 5 | play -t raw -b 16 -c 1 -e s -r 8820 -
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("rate教えろ\n");
    exit(1);
  }

  const int rate = atoi(argv[1]);

  int16_t buf;
  int n;
  int count = 0;
  while ((n = read(STDIN_FILENO, &buf, sizeof(int16_t))) > 0) {
    if (count % rate == 0) {
      int _ = write(STDOUT_FILENO, &buf, 2);
    }
    ++count;
  }

  if (n < 0) {
    perror("read");
    exit(1);
  }

  return 0;
}