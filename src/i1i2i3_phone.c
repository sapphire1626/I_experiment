#include <arpa/inet.h>
#include <fcntl.h>
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
#include "lib/params.h"
#include "lib/setup_socket.h"

void finish(const char* cmd);
void* send_thread_func(void* arg);
FILE* fp;

int is_port_muted(int port, int* muted_ports, int muted_count) {
  for (int i = 0; i < muted_count; i++) {
    if (muted_ports[i] == port) return 1;
  }
  return 0;
}

/// @param argv[1] サーバIPアドレス
/// @param argv[2] オプションでWAVファイル名
/// @param -m: muteオプション（音声入力をミュート）
/// @param -h:
/// holdオプション（データを送信しなくてもサーバから切断されなくなる）
int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Usage: %s <server address> [wavfile]\n", argv[0]);
    return 1;
  }

  const char* ip_addr = argv[1];

  uint8_t hold = 0;
  uint8_t mute = 0;
  for (int i = 2; i < argc; i++) {
    if (argv[i][0] != '-') {
      continue;
    }

    switch (argv[i][1]) {
      case 'h':
        hold = 1;  // -hオプションでholdを有効にする
        break;
      case 'm':
        mute = 1;  // -mオプションでmuteを有効にする
        hold = 1;
        break;
      default:
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return 1;
    }
  }

  setup(ip_addr, hold);

  // 送信スレッド作成
  pthread_t send_thread;
  if (mute == 0) {
    void* arg = NULL;
    if (argc == 3) {
      arg = argv[2];
    }
    if (pthread_create(&send_thread, NULL, send_thread_func, arg) != 0) {
      finish("pthread_create");
    }
  } else {
    // 送信しないとそもそもサーバに認識してもらえないので何かしら送る
    char dummy_buf[DATA_SIZE] = {0};
    int dummy_len = encode(dummy_buf, sizeof(dummy_buf), dummy_buf);
    if (sendData(dummy_buf, dummy_len) < 0) {
      finish("send dummy data");
    }
  }

  int num_clients = 0;
  char recv_buf[DATA_SIZE * (MAX_PORT - GATE_PORT)];
  int length_list[MAX_PORT - GATE_PORT];
  int port_list[MAX_PORT - GATE_PORT];
  char decoded_bufs[MAX_PORT - GATE_PORT]
                   [DATA_SIZE];  // 全員のデコード済みデータ
  short* pcm_ptrs[MAX_PORT - GATE_PORT];
  short mix_buf[DATA_SIZE / 2];  // ミキシング用
  int muted_ports[MAX_PORT - GATE_PORT] = {0};
  int muted_count = 0;
  // 標準入力をノンブロッキングに
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  while (1) {
    // --- コマンド入力監視 ---
    char cmd_buf[64];
    if (fgets(cmd_buf, sizeof(cmd_buf), stdin)) {
      int port;
      if (sscanf(cmd_buf, "m %d", &port) == 1) {
        // ミュート追加
        int already = 0;
        for (int i = 0; i < muted_count; i++) {
          if (muted_ports[i] == port) already = 1;
        }
        if (!already && muted_count < MAX_PORT - GATE_PORT) {
          muted_ports[muted_count++] = port;
          fprintf(stderr, "[MUTE] port %d\n", port);
          fflush(stderr);
        }
      } else if (sscanf(cmd_buf, "u %d", &port) == 1) {
        // ミュート解除
        for (int i = 0; i < muted_count; i++) {
          if (muted_ports[i] == port) {
            for (int j = i; j < muted_count - 1; j++) {
              muted_ports[j] = muted_ports[j + 1];
            }
            muted_count--;
            fprintf(stderr, "[UNMUTE] port %d\n", port);
            fflush(stderr);
            break;
          }
        }
      } else if (strncmp(cmd_buf, "list", 4) == 0) {
        // ミュートポート一覧表示
        fprintf(stderr, "[PORTS LIST]");
        for (int i = 0; i < num_clients; i++) {
          fprintf(stderr, " %d", port_list[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
      }
    }
    // 受信
    num_clients = receiveData(recv_buf, length_list, port_list);
    int max_samples = 0;
    int mix_count = 0;
    for (int i = 0; i < num_clients; i++) {
      int decoded_len =
          decode(recv_buf + DATA_SIZE * i, length_list[i], decoded_bufs[i]);
      int nsamp = decoded_len / 2;
      if (nsamp > max_samples) max_samples = nsamp;
      if (!is_port_muted(port_list[i], muted_ports, muted_count)) {
        pcm_ptrs[mix_count++] = (short*)decoded_bufs[i];
      }
    }
    if (mix_count > 0) {
      mixing(pcm_ptrs, mix_count, max_samples, mix_buf);
      if (write(STDOUT_FILENO, mix_buf, max_samples * 2) < 0) {
        finish("write");
      }
    }
  }

  // スレッド終了待ち
  pthread_cancel(send_thread);
  pthread_join(send_thread, NULL);

  finish(NULL);
}

// 送信スレッド用関数
void* send_thread_func(void* arg) {
  char* cmdline;
  if (arg == NULL) {
    // デフォルト: rec
    cmdline = "rec -t raw -b 16 -c 1 -e s -r 44100 -";
    printf("Using rec for audio input\n");
  } else {
    // WAVファイル指定時: soxでraw変換
    char* wavfile = (char*)arg;
    static char buf[256];
    snprintf(buf, sizeof(buf), "sox '%s' -t raw -b 16 -c 1 -e s -r 44100 -",
             wavfile);
    cmdline = buf;
    printf("Using %s for audio input\n", wavfile);
  }

  FILE* fp_send = popen(cmdline, "r");
  if (fp_send == NULL) {
    perror("can not exec command");
    finish("popen");
  }

  char buf[DATA_SIZE];
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
      char encoded_buf[DATA_SIZE];
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