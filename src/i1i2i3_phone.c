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
int main(int argc, char* argv[]) {
  //   const char* ip_addr = argv[1];
  //   const int recv_port = atoi(argv[2]);
  //   const int send_port = atoi(argv[3]);
  //   if (argc != 4) {
  //     printf("Usage: %s <ip_addr> <recv_port> <send_port>\n", argv[0]);
  //     return 1;
  //   }

  struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
  //   s_recv = setUpSocketTcp(&addr_recv, ip_addr, recv_port);

  //   struct sockaddr_in addr_send;
  //   socklen_t addr_len_send = sizeof(struct sockaddr_in);
  //   s_send = setUpSocketTcpServer(&addr_send, &addr_len_send, send_port);
  int s;
  if (argc == 2) {
    const int port = atoi(argv[1]);
    s = setUpSocketTcpServer(&addr, &addr_len, port);
  } else if (argc == 3) {
    const char* ip_addr = argv[1];
    const int port = atoi(argv[2]);
    s = setUpSocketTcp(&addr, ip_addr, port);
  } else {
    printf("Usage: %s <port> or %s <ip_addr> <port>\n", argv[0], argv[0]);
    exit(EXIT_FAILURE);
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
      if (write(s, buf, c) < 0) {
        finish("write");
      }
    }

    // 受信
    c = read(s, buf, sizeof(buf));
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