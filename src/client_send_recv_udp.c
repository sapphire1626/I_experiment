#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "lib/setup_socket.h"

int checkAllOf(char arr[], int size, char str) {
  for (int i = 0; i < size; i++) {
    if (arr[i] != str) {
      return 0;  // 一つでも違う文字列があればfalse
    }
  }
  return 1;  // 全て同じ文字列ならtrue
}

void makeEOD(char arr[], int size) {
  for (int i = 0; i < size; i++) {
    arr[i] = 0b11111111;  // 全て1にする
  }
}

/// @brief
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

  struct sockaddr_in addr_in;
  int s = setUpSocketUdp(&addr_in, ip_addr, port);
  struct sockaddr* addr = (struct sockaddr*)&addr_in;
  socklen_t addr_len = (socklen_t)sizeof(addr_in);

  // 最初に１バイト送信
  //   sendto(s, "X", 1, 0, addr, addr_len);

  char buf[1000];
  int c;

  // 送信
  // １回1000バイトまで、最大50回
  int count = 0;
  while ((c = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
    if (++count > 50) {
      break;  // 最大50回送信
    }
    if (sendto(s, buf, c, 0, addr, addr_len) < 0) {
      perror("sendto");
      return 1;
    }
  }
  if (c < 0) {
    perror("read");
    return 1;
  }

  makeEOD(buf, sizeof(buf));  // 全て1にする
  if (sendto(s, buf, sizeof(buf), 0, addr, addr_len) < 0) {
    perror("sendto");
    return 1;
  }

  shutdown(s, SHUT_WR);  // 送信はこれ以上しないことを相手に通知

  // 受信
  while ((c = recvfrom(s, buf, sizeof(buf), 0, addr, &addr_len)) > 0) {
    if (c == sizeof(buf) && checkAllOf(buf, c, 0b11111111)) {
      break;  // 受信したデータが全て1なら終了
    }
    if (write(STDOUT_FILENO, buf, c) < 0) {
      perror("write");
      return 1;
    }
  }
  if (c < 0) {
    perror("recvfrom");
    return 1;
  }

  close(s);
  return 0;
}