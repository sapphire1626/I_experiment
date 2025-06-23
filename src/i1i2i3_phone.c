#include <arpa/inet.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
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
    printf("Usage: %s <server address> <server port>\n", argv[0]);
    return 1;
  }
  setup(ip_addr, port);

  // 送信スレッド作成
  pthread_t send_thread;
  if (pthread_create(&send_thread, NULL, send_thread_func, NULL) != 0) {
    perror("pthread_create");
    finish("pthread_create");
  }

  char recv_buf[BUFFER_SIZE];
  int c;
  while (1) {
    // 受信
    c = receiveData(recv_buf, sizeof(recv_buf));
    if (c == 0) {
      break;
    }
    // if (r < 0) {
    //   finish("read encoded_len");
    // }
    // 次にencoded_lenバイト分受信
    // int received = 0;
    // while (received < encoded_len) {
    //   int n = read(s_recv, recv_buf + received, encoded_len - received);
    //   if (n <= 0) finish("read encoded data");
    //   received += n;
    // }
    char decoded_buf[BUFFER_SIZE];
    int decoded_len = decode(recv_buf, c, decoded_buf);
    if (write(STDOUT_FILENO, decoded_buf, decoded_len) < 0) {
      finish("write");
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
    perror("can not exec command");
    finish("popen");
  }
  char buf[BUFFER_SIZE];
  int c;
  // // ここから
  // static int seeded = 0;
  // if (!seeded) { srand((unsigned)time(NULL)); seeded = 1; }
  // // ここまで
  while (1) {
    c = fread(buf, sizeof(buf[0]), sizeof(buf) / sizeof(buf[0]), fp_send);
    if (c < 0) {
      pclose(fp_send);
      finish("fread");
    }
    if (c > 0) {
      // // --- ノイズ付加 ---
      // short* pcm = (short*)buf;
      // int nsamp = c / 2; // short型PCMサンプル数
      // int NOISE_LEVEL = 1000; // ノイズ強度（調整可）
      // for (int i = 0; i < nsamp; i++) {
      //   int noise = (rand() % (2 * NOISE_LEVEL)) - NOISE_LEVEL; //
      //   -NOISE_LEVEL～+NOISE_LEVEL int v = pcm[i] + noise; if (v > 32767) v =
      //   32767; if (v < -32768) v = -32768; pcm[i] = (short)v;
      // }
      // // --- ノイズ付加ここまで ---
      char encoded_buf[BUFFER_SIZE];
      int encoded_len = encode(buf, c, encoded_buf);

      if (sendData(encoded_buf, encoded_len) < 0) {
        pclose(fp_send);
        finish("write encoded_buf");
      }
    }
  }
  pclose(fp_send);
  return NULL;
}