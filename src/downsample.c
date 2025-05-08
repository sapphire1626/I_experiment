#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 256

void die(char *s) {
  perror(s);
  exit(1);
}

/// @example sox ./data/obama.wav -t raw -b 16 -c 1 -e s -r 44100 - |
/// ./build/downsample 5 | play -t raw -b 16 -c 1 -e s -r 8820 -
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("rate教えろ\n");
    exit(1);
  }

  int rate = atoi(argv[1]);

  int16_t buf[N];

  int n;
  int i = 0;
  while (1) {
    n = read(STDIN_FILENO, buf, 2);

    if (n == -1) {
      die("read");
    }
    if (n == 0) {
      break;
    }

    if (i % rate == 0) {
      write(STDOUT_FILENO, &buf[0], 2);
    }
    ++i;
  }

  return 0;
}