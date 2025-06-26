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
/// @param -l: 音声再生を行う
/// @param -s: 音声送信を行う
/// holdオプション（データを送信しなくてもサーバから切断されなくなる）
int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Usage: %s <server address> [wavfile]\n", argv[0]);
    return 1;
  }

  const char* ip_addr = argv[1];

  uint8_t hold = 1;
  uint8_t speaking = 0;
  uint8_t listening = 0;
  char* wavfile = NULL;
  for (int i = 2; i < argc; i++) {
    if (argv[i][0] != '-') {
      wavfile = argv[i];
      speaking = 1;
      continue;
    }

    switch (argv[i][1]) {
      case 'l':
        listening = 1;
        hold = 1;
        break;
      case 's':
        speaking = 1;
        hold = 0;
        break;
      default:
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        return 1;
    }
  }

  if (listening == 0 && speaking == 0) {
    fprintf(stderr, "Error: At least one of -l or -s must be specified.\n");
    return 1;
  }

  setup(ip_addr, hold);

  // 送信スレッド作成
  pthread_t send_thread;
  if (speaking == 1) {
    void* arg = wavfile;
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
  int long_life_port_list[MAX_PORT - GATE_PORT];
  int port_life[MAX_PORT - GATE_PORT] = {0};  // ポートの生存時間
  int long_life_num_clients = 0;
  const int LIFE = 10000000;
  char decoded_bufs[MAX_PORT - GATE_PORT]
                   [DATA_SIZE];  // 全員のデコード済みデータ
  short* pcm_ptrs[MAX_PORT - GATE_PORT];
  short mix_buf[DATA_SIZE / 2];  // ミキシング用
  int muted_ports[MAX_PORT - GATE_PORT] = {0};
  int muted_count = 0;
  // 標準入力をノンブロッキングに
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  while (listening == 1) {
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
        for (int i = 0; i < long_life_num_clients; i++) {
          fprintf(stderr, " %d", long_life_port_list[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
      }
    }
    // 受信
    num_clients = receiveData(recv_buf, length_list, port_list);

    // 長期持続port_listの更新
    for (int i = 0; i < num_clients; i++) {
      const int port = port_list[i];
      int found = 0;
      for (int j = 0; j < long_life_num_clients; j++) {
        if (long_life_port_list[j] == port) {
          found = 1;
          port_life[j] = LIFE;  // 生存時間リセット
          break;
        }
      }
      if (!found && long_life_num_clients < MAX_PORT - GATE_PORT) {
        // 新規ポート追加
        long_life_num_clients++;
        long_life_port_list[long_life_num_clients - 1] = port;
        port_life[long_life_num_clients - 1] = LIFE;
      }
    }
    // 生存時間の更新
    for (int i = 0; i < long_life_num_clients; i++) {
      port_life[i]--;
      if (port_life[i] <= 0) {
        // ポートが死んだ
        fprintf(stderr, "[PORT DEAD] port %d\n", long_life_port_list[i]);
        fflush(stderr);
        // ミュートポートから削除
        for (int j = 0; j < muted_count; j++) {
          if (muted_ports[j] == long_life_port_list[i]) {
            for (int k = j; k < muted_count - 1; k++) {
              // １個ずつ前に詰める
              muted_ports[k] = muted_ports[k + 1];
            }
            muted_count--;
            break;
          }
        }
        // ポートリストから削除
        for (int j = i; j < long_life_num_clients - 1; j++) {
          // １個ずつ前に詰める
          long_life_port_list[j] = long_life_port_list[j + 1];
          port_life[j] = port_life[j + 1];
        }
        long_life_num_clients--;
        i--;  // 削除したのでインデックスを戻す
      }
    }
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
  // pthread_cancel(send_thread);
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
  fprintf(stderr, "Finishing... (with cmd: %s)\n", cmd);
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