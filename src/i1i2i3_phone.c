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
#include "lib/network.h"
#include "lib/setup_socket.h"

void* send_thread_func(void* arg);
FILE* fp;

void finish(const char* cmd) {
  if (cmd != NULL) {
    perror(cmd);
  }
  cleanUp();
  pclose(fp);

  if (cmd != NULL) {
    exit(EXIT_FAILURE);
  } else {
    exit(EXIT_SUCCESS);
  }
}

///@brief
///@param argv[1] サーバIPアドレス
///@param argv[2] サーバポート
int main(int argc, char* argv[]) {
  const char* ip_addr = argv[1];
  const int port = atoi(argv[2]);
  if (argc != 3) {
    printf("Usage: %s <server address> <server port>\n",
           argv[0]);
    return 1;
  }
  // const char* protocol = "tcp";
  // if (strcmp(mode, "server") == 0) {
    // setupReceive(ip_addr, recv_port, protocol);
    // setupSend(ip_addr, send_port, protocol);
  // } else {
    // setupSend(ip_addr, send_port, protocol);
    // setupReceive(ip_addr, recv_port, protocol);
  // }
  setup(ip_addr, port);

  // 送信スレッド作成
  pthread_t send_thread;
  if (pthread_create(&send_thread, NULL, send_thread_func, NULL) != 0) {
    perror("pthread_create");
    finish("pthread_create");
  }

  char buf[256];
  int c;
  while (1) {
    // 受信
    c = receiveData(buf, sizeof(buf));
    // c = read(s_recv, buf, sizeof(buf));
    if (c == 0) {
      break;
    }
    if (c > 0) {
      char decoded_buf[1024];
      int decoded_len = decode(buf, c, decoded_buf);
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
  // char* cmdline = "rec -t raw -b 16 -c 1 -e s -r 44100 -";
  // FILE* fp_send = popen(cmdline, "r");
  // if (fp_send == NULL) {
  //   perror("can not exec command");
  //   finish("popen");
  // }
  char buf[256];
  int c;
  while (1) {
    // c = fread(buf, sizeof(buf[0]), sizeof(buf) / sizeof(buf[0]), fp_send);
    c = read(STDIN_FILENO, buf, sizeof(buf));
    if (c < 0) {
      // pclose(fp_send);
      finish("fread");
    }
    if (c > 0) {
      char encoded_buf[512];
      int encoded_len = encode(buf, c, encoded_buf);

      // if (write(s_send, encoded_buf, encoded_len) < 0) {
      if (sendData(encoded_buf, encoded_len) < 0) {
        // pclose(fp_send);
        finish("write");
      }
    }
  }
  // pclose(fp_send);
  return NULL;
}