#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "lib/params.h"

enum PortStatus {
  PORT_FREE = 0,  // 当該ポートは未使用
  PORT_WAIT_CONNECT = 1,  // 当該ポートで最初のアクセスが来るのを待機している
  PORT_CONNECTED = 2,  // 当該ポートの接続先アドレスが判明している
};

typedef struct {
  enum PortStatus status;
  int socket;
  struct sockaddr_in addr;
  socklen_t addr_len;
} client_info_t;

typedef struct {
  client_info_t clients[MAX_PORT - GATE_PORT];
} client_table_t;

void init_client_table(client_table_t *table) {
  for (int i = 0; i < MAX_PORT - GATE_PORT; ++i) {
    table->clients[i].status = PORT_FREE;
    table->clients[i].socket = -1;
    memset(&table->clients[i].addr, 0, sizeof(table->clients[i].addr));
    table->clients[i].addr_len = sizeof(table->clients[i].addr);
  }
}

int find_free_client(client_table_t *table) {
  for (int i = 1; i < MAX_PORT - GATE_PORT; ++i) {
    if (table->clients[i].status == PORT_FREE) return GATE_PORT + i;
  }
  return -1;
}

void udp_server(int port, client_table_t *client_table, uint8_t hold) {
  const int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  const int table_index = port - GATE_PORT;
  if (sockfd < 0) {
    perror("socket");
    exit(1);
  }

  if (hold == 0) {
    // タイムアウト設定（例: 10秒）
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  }

  // 待ち受けソケットを設定してsockfdにbind
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("bind");
    close(sockfd);
    exit(1);
  }

  client_table->clients[port - GATE_PORT].socket = sockfd;
  printf("port %d bound to socket %d\n", port,
         client_table->clients[table_index].socket);

  char buf[DATA_SIZE];
  while (1) {
    // 受信
    const ssize_t n =
        recvfrom(sockfd, buf, DATA_SIZE, 0,
                 (struct sockaddr *)&client_table->clients[table_index].addr,
                 &client_table->clients[table_index].addr_len);
    if (n < 0) {
      // タイムアウトまたはエラー
      perror("recvfrom (timeout or error)");
      break;
    }

    client_table->clients[table_index].status = PORT_CONNECTED;

    // 送信
    const int num_clients = MAX_PORT - GATE_PORT;
    for (int i = 1; i < num_clients; i++) {
      if (client_table->clients[i].status != PORT_CONNECTED) {
        // printf("not used client %d\n", i + GATE_PORT);
        continue;
      }
      if (i == table_index) {
        // printf("skipping self client %d\n", i + GATE_PORT);
        continue;  // 自分自身には送信しない
      }
      if (client_table->clients[i].socket < 0) {
        printf("invalid socket for client %d\n", i + GATE_PORT);
        continue;  // ソケットが無効な場合はスキップ
      }

      // printf("sending to client port = %d, socket = %d\n", i + GATE_PORT,
      //  client_table->clients[i].socket);
      if (sendto(client_table->clients[i].socket, buf, n, 0,
                 (struct sockaddr *)&client_table->clients[i].addr,
                 client_table->clients[i].addr_len) < 0) {
        perror("sendto");
      }
    }
  }
  close(sockfd);
  client_table->clients[table_index].status = PORT_FREE;
  printf("client at port %d disconnected (timeout or error)\n", port);
}

int gate_sock = -1, gate_newsock = -1;

void on_finish() {
  if (gate_newsock >= 0) {
    close(gate_newsock);
  }
  if (gate_sock >= 0) {
    close(gate_sock);
  }
  exit(0);
}

typedef struct {
  int port;
  client_table_t *client_table;
  uint8_t hold;
} udp_server_arg_t;

void *udp_server_thread_fn(void *arg) {
  udp_server_arg_t *server_arg = (udp_server_arg_t *)arg;
  int port = server_arg->port;
  client_table_t *client_table = server_arg->client_table;
  uint8_t hold = server_arg->hold;
  free(server_arg);
  udp_server(port, client_table, hold);
  return NULL;
}

int main() {
  // シグナルハンドラで確実にソケットを閉じる
  signal(SIGINT, on_finish);

  client_table_t client_table;
  init_client_table(&client_table);

  // GATE_PORTはTCPで待ち受け
  if ((gate_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }
  struct sockaddr_in gate_addr;
  memset(&gate_addr, 0, sizeof(gate_addr));
  gate_addr.sin_family = AF_INET;
  gate_addr.sin_addr.s_addr = INADDR_ANY;
  gate_addr.sin_port = htons(GATE_PORT);
  if (bind(gate_sock, (struct sockaddr *)&gate_addr, sizeof(gate_addr)) < 0) {
    perror("bind");
    close(gate_sock);
    exit(1);
  }
  listen(gate_sock, 5);
  printf("Gate TCP server listening on port %d\n", GATE_PORT);

  while (1) {
    // 受付
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    if ((gate_newsock =
             accept(gate_sock, (struct sockaddr *)&cli_addr, &clilen)) < 0) {
      perror("accept");
      continue;
    }

    // 使用ポートの割り当て
    int p = find_free_client(&client_table);
    if (p == -1) {
      fprintf(stderr, "No free port available\n");
      close(gate_newsock);
      continue;
    }
    client_table.clients[p - GATE_PORT].status = PORT_WAIT_CONNECT;

    // ポート番号をクライアントに送信
    uint16_t port_u16 = htons(p);  // バイトオーダの調整
    write(gate_newsock, &port_u16, sizeof(port_u16));

    // hold指令を受け取る
    uint8_t hold;
    if (read(gate_newsock, &hold, sizeof(hold)) < 0) {
      perror("read");
      continue;
    }

    close(gate_newsock);

    // スレッドでUDP echoサーバ起動
    pthread_t tid;
    udp_server_arg_t *arg = malloc(sizeof(udp_server_arg_t));
    arg->port = p;
    arg->client_table = &client_table;
    arg->hold = hold;
    if (pthread_create(&tid, NULL, udp_server_thread_fn, arg) != 0) {
      perror("pthread_create");
      free(arg);
      continue;
    }
    pthread_detach(tid);  // 終了時に自動回収
  }
  close(gate_sock);
  return 0;
}
