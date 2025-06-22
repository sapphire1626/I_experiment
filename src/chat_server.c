#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define GATE_PORT 16260
#define MAX_PORT 17000
#define BUF_SIZE 1024

typedef struct {
  int used;
  int socket;
  struct sockaddr addr;
} client_info_t;

typedef struct {
  client_info_t clients[MAX_PORT - GATE_PORT];
} client_table_t;

void init_client_table(client_table_t *table) {
  for (int i = 0; i < MAX_PORT - GATE_PORT; ++i) {
    table->clients[i].used = 0;
    table->clients[i].socket = -1;
    memset(&table->clients[i].addr, 0, sizeof(table->clients[i].addr));
  }
}

int find_free_client(client_table_t *table) {
  for (int i = 1; i < MAX_PORT - GATE_PORT; ++i) {
    if (!table->clients[i].used) return GATE_PORT + i;
  }
  return -1;
}

void set_client_used(client_table_t *table, int port, int used) {
  if (port > GATE_PORT && port < MAX_PORT) {
    table->clients[port - GATE_PORT].used = used;
  }
}

void udp_echo_server(int port, client_table_t *client_table) {
  const int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(1);
  }
  client_table->clients[port - GATE_PORT].socket = sockfd;

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

  char buf[BUF_SIZE];
  socklen_t clilen = sizeof(client_table->clients[port - GATE_PORT].addr);
  while (1) {
    // 受信
    ssize_t n =
        recvfrom(sockfd, buf, BUF_SIZE, 0,
                 &client_table->clients[port - GATE_PORT].addr, &clilen);
    if (n < 0) break;

    // 送信
    sendto(sockfd, buf, n, 0, &client_table->clients[port - GATE_PORT].addr,
           clilen);
  }
  close(sockfd);
  exit(0);
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

int main() {
  // シグナルハンドラで確実にソケットを閉じる
  signal(SIGINT, on_finish);

  struct sockaddr_in gate_addr, cli_addr;
  client_table_t client_table;
  init_client_table(&client_table);

  // GATE_PORTはTCPで待ち受け
  if ((gate_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }
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
    set_client_used(&client_table, p, 1);

    // ポート番号をクライアントに送信
    uint16_t port_u16 = htons(p);  // バイトオーダの調整
    write(gate_newsock, &port_u16, sizeof(port_u16));
    close(gate_newsock);

    // 子プロセスでUDP echoサーバ起動
    pid_t pid = fork();
    if (pid == 0) {
      udp_echo_server(p, &client_table);
      set_client_used(&client_table, p, 0);  // 終了時に解放
      exit(0);
    }
  }
  close(gate_sock);
  return 0;
}
