
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define N 256
/**
 * ファイルパスをうけとり、バイト列をインデックス付きで出力する
 * @example ./build/read_data ./data/sound.raw > ./data/sound.data
 */
int main(int argc, char* argv[]) {
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  int cnt = 0;
  int read_res;
  unsigned char buf[N];
  while ((read_res = read(fd, &buf, N)) > 0) {
    for(int i = 0; i < read_res; i++){
      printf("%d %d\n", cnt++, buf[i]);
    }
  }
  if (read_res < 0) {
    perror("read");
    close(fd);
    return 1;
  }

  close(fd);
  return 0;
}