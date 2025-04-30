
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

/**
 * ファイルパスをうけとり、バイト列をインデックス付きで出力する
 */
int main(int argc, char* argv[]) {
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  int cnt = 0;
  int read_res;
  int16_t buf;
  while ((read_res = read(fd, &buf, 2)) > 0) {
    printf("%d %d\n", cnt++, buf);
  }
  if (read_res < 0) {
    perror("read");
    close(fd);
    return 1;
  }
  close(fd);
  return 0;
}