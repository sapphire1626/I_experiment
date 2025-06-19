#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pthread.h>  // 追加
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "lib/encode.h"
#include "lib/setup_socket.h"

void* send_thread_func(void* arg);
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

  // 送信スレッド作成
  pthread_t send_thread;
  if (pthread_create(&send_thread, NULL, send_thread_func, NULL) != 0) {
    perror("pthread_create");
    finish("pthread_create");
  }

  char recv_buf[2048]; // 2048本当に？
  int c;
  while (1) {
    // 受信
    c = read(s_recv, recv_buf, sizeof(recv_buf));
    if (c == 0) {
      break;
    }
    if (c > 0) {
      char decoded_buf[1024];
      int decoded_len = decode(recv_buf, c, decoded_buf);
      if (write(STDOUT_FILENO, decoded_buf, decoded_len) < 0) {
        finish("write");
      }
    } else if (c < 0) {
      finish("read");
    }
  }

  // スレッド終了待ち
  pthread_cancel(send_thread);
  pthread_join(send_thread, NULL);

  finish(NULL);
}

// 送信スレッド用関数
void* send_thread_func(void* arg) {
  char* cmdline = "rec -t raw -b 16 -c 1 -e s -r 44100 -";
  FILE* fp_send = popen(cmdline, "r");
  if (fp_send == NULL) {
    perror("can not exec commad");
    finish("popen");
  }
  char buf[256];
  int c;
  while (1) {
    c = fread(buf, sizeof(buf[0]), sizeof(buf) / sizeof(buf[0]), fp_send);
    if (c < 0) {
      pclose(fp_send);
      finish("fread");
    }
    if (c > 0) {
      char encoded_buf[2048]; // 2048本当に？
      int encoded_len = encode(buf, c, encoded_buf);

      if (write(s_send, encoded_buf, encoded_len) < 0) {
        pclose(fp_send);
        finish("write");
      }
    }
  }
  pclose(fp_send);
  return NULL;
}