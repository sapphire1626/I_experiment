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
/// @param addr
/// @param ip_addr
/// @param port
/// @return socketのファイルディスクリプタを返す
int setUpSocket(struct sockaddr_in* addr, const char* ip_addr, int port);

/// IPアドレスとポート番号を指定して接続してEOFまで標準入力の内容を送信したあと、送られてきたデータを標準出力に出す
/// @param argv[1] IPアドレス
/// @param argv[2] ポート番号
int main(int argc, char* argv[]) {
  // UDP => SOCK_DGRAM
  // TCP => SOCK_STREAM

  if (argc != 3) {
    printf("IPアドレスとポート教えろ");
  }

  char* ip_addr = argv[1];
  int port = atof(argv[2]);

  struct sockaddr_in addr;
  int s = setUpSocket(&addr, ip_addr, port);

  char buf[256];
  int c;
  // 送信
  while (c = read(STDIN_FILENO, buf, sizeof(buf))) {
    if (write(s, buf, c) < 0) {
      perror("write");
      return 1;
    }
  }
  if (c < 0) {
    perror("read");
    return 1;
  }

  shutdown(s, SHUT_WR);  // 送信はこれ以上しないことを相手に通知

  // 受信
  while (c = read(s, buf, sizeof(buf))) {
    if (write(STDOUT_FILENO, buf, c) < 0) {
      perror("write");
      return 1;
    }
  }
  if (c < 0) {
    perror("read");
    return 1;
  }

  close(s);
  return 0;
}

int setUpSocket(struct sockaddr_in* addr, const char* ip_addr, int port) {
  int s = socket(PF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    perror("socket");
    exit(1);
  }

  addr->sin_family = AF_INET;  // IPv4
  if (inet_aton(ip_addr, &addr->sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }

  addr->sin_port = htons(port);

  printf("connecting...\n");
  int ret = connect(s, (struct sockaddr*)addr, sizeof(*addr));
  if (ret < 0) {
    perror("connect");
    exit(1);
  }
  printf("connected\n");

  return s;
}