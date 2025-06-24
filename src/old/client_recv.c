#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/// @brief
/// IPアドレスとポート番号を指定して接続して送られてきたデータを標準出力に出す
/// @param argv[1] IPアドレス
/// @param argv[2] ポート番号
int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: %s <ip_address> <port>\n", argv[0]);
    return 1;
  }

  char* ip_addr = argv[1];
  int port = atof(argv[2]);

  // 接続
  int s = socket(PF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;  // IPv4
  if (inet_aton(ip_addr, &addr.sin_addr) == 0) {
    perror("inet_aton");
    return 1;
  }
  addr.sin_port = htons(port);

  int ret = connect(s, (struct sockaddr*)&addr, sizeof(addr));
  if (ret < 0) {
    perror("connect");
    return 1;
  }

  // データ受信
  char buf[256];
  int c;
  while ((c = read(s, buf, sizeof(buf))) > 0) {
    int _ = write(STDOUT_FILENO, buf, c);
  }
  if (c < 0) {
    perror("read");
    return 1;
  }

  close(s);
  return 0;
}