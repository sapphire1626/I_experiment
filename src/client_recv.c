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
  // UDP => SOCK_DGRAM
  // TCP => SOCK_STREAM

  if (argc != 3) {
    printf("IPアドレスとポート教えろ");
  }

  char* ip_addr = argv[1];
  int port = atof(argv[2]);

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
  printf("connetinng...\n");
  int ret = connect(s, (struct sockaddr*)&addr, sizeof(addr));
  if (ret < 0) {
    perror("connect");
    return 1;
  } else {
    printf("connected\n");
  }

  char buf[256];
  int c;
  while (c = read(s, buf, sizeof(buf))) {
    int _ = write(STDOUT_FILENO, buf, c);
  }
  if (c < 0) {
    perror("read");
    return 1;
  }

  // if (argc == 1) {
  //   printf("listening...\n");
  //   int N = 256;
  //   char data[N];
  //   int n = read(s, data, N);
  //   if (n == -1) {
  //     perror("read");
  //     return 1;
  //   }
  //   printf("read %d bytes\n", n);
  //   printf("data: %s\n", data);
  // } else {
  //   // sender
  //   printf("sending...\n");
  //   const char *msg = "hello world";
  //   int n = send(s, msg, strlen(msg)+1, 0);
  //   if (n < 0) {
  //     perror("send");
  //     return 1;
  //   } else {
  //     printf("sent %d bytes\n", n);
  //   }
  // }

  close(s);
  return 0;
}