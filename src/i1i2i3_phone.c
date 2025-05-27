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

FILE* fp;
int s_recv;
int s_send;

void finish(const char* cmd) {
  if (cmd != NULL) {
    perror(cmd);
  }
  if (s_recv >= 0) {
    close(s_recv);
  }
  if (s_send >= 0) {
    close(s_send);
  }
  pclose(fp);

  if (cmd != NULL) {
    exit(EXIT_FAILURE);
  } else {
    exit(EXIT_SUCCESS);
  }
}

///@brief
///@param argv[1] 通信先IPアドレス
///@param argv[2] 通信先ポート番号
///@param argv[3] 受信ポート番号
///@param argv[4] server or client
int main(int argc, char* argv[]) {
  const char* ip_addr = argv[1];
  const int recv_port = atoi(argv[2]);
  const int send_port = atoi(argv[3]);
  const char* mode = argv[4];
  if (argc != 5) {
    printf("Usage: %s <ip_addr> <recv_port> <send_port> <server/client>\n",
           argv[0]);
    return 1;
  }

  if (strcmp(mode, "server") != 0 && strcmp(mode, "client") != 0) {
    printf("mode must be 'server' or 'client'\n");
    return 1;
  }

  struct sockaddr_in addr_recv;
  struct sockaddr_in addr_send;
  socklen_t addr_len_send = sizeof(struct sockaddr_in);
  if (strcmp(mode, "server") == 0) {
    s_send = setUpSocketTcpServer(&addr_send, &addr_len_send, send_port);
    s_recv = setUpSocketTcp(&addr_recv, ip_addr, recv_port);
  } else {
    s_recv = setUpSocketTcp(&addr_recv, ip_addr, recv_port);
    s_send = setUpSocketTcpServer(&addr_send, &addr_len_send, send_port);
  }

  char* cmdline = "rec -t raw -b 16 -c 1 -e s -r 44100 -";
  if ((fp = popen(cmdline, "r")) == NULL) {
    perror("can not exec commad");
    exit(EXIT_FAILURE);
  }

  char buf[256];
  int c;
  while (1) {
    // 送信パート
    c = fread(buf, sizeof(buf[0]), sizeof(buf) / sizeof(buf[0]), fp);
    if (c < 0) {
      finish("fread");
    }

    if (c > 0) {
      if (write(s_send, buf, c) < 0) {
        finish("write");
      }
    }

    // 受信
    c = read(s_recv, buf, sizeof(buf));
    if (c == 0) {
      break;
    }
    if (c > 0) {
      if (write(STDOUT_FILENO, buf, c) < 0) {
        finish("write");
      }
    } else if (c < 0) {
      finish("read");
    }
  }

  finish(NULL);
}