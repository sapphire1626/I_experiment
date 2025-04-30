
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
/**
 * ファイルパスをうけとり、バイト列をインデックス付きで出力する
 */
int main(int argc, char* argv[]){
    char buf;
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0){
        perror("open");
        return 1;
    }

    int cnt = 0;
    int read_res;
    while ((read_res = read(fd, &buf, 1)) > 0){
        printf("%d %d\n", cnt++, buf);
    }
    if (read_res < 0){
        perror("read");
        close(fd);
        return 1;
    }
    if (read_res == 0){
        printf("Success: %d lines\n", cnt);
    }

    close(fd);
    return 0;
}