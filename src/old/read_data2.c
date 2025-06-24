
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#define N 256

/**
 * ファイルパスをうけとり、２バイト列をインデックス付きで出力する
 * @example ./build/read_data2 ./data/doremi.raw > ./data/doremi.data
 */
int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
    return 1;
  }
  const char* filepath = argv[1];

  int fd = open(filepath, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  int cnt = 0;
  int read_res;
  int16_t buf[N];
  while ((read_res = read(fd, &buf, sizeof(buf))) > 0) {
    for (int i = 0; i < read_res / sizeof(int16_t); i++) {
      printf("%d %d\n", cnt++, buf[i]);
    }
  }

  close(fd);

  if (read_res < 0) {
    perror("read");
    return 1;
  }

  return 0;
}