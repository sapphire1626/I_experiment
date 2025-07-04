#include "network.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "params.h"
#include "setup_socket.h"

#define RTP_HEADER_SIZE 12
#define RING_SIZE 1000

void storeDataBuffer(int port, const uint8_t* data, int len);
int getDataBuffer(int port, uint8_t* data);
int communication_sock = -1;
struct sockaddr_in addr_recv;
struct sockaddr_in addr_send;
pthread_t data_receive_thread;
int communication_port = -1;
static uint16_t data_seq_latest[MAX_PORT - GATE_PORT] = {0};

typedef struct {
  uint8_t vpxcc;       // version(2), padding(0), extension(0), CSRC count(0)
  uint8_t mpt;         // marker(0), payload type(0)
  uint16_t seq;        // sequence number
  uint32_t timestamp;  // timestamp
  uint32_t ssrc;       // synchronization source identifier
} __attribute__((packed)) RTPHeader;

void makeRTPHeader(RTPHeader* header, uint16_t seq, uint32_t ts,
                   uint32_t ssrc) {
  header->vpxcc = 0x80;  // Version 2, no padding, no extension, CC=0
  header->mpt = 0x0B;    // M=0, PT=11
  header->seq = htons(seq);
  header->timestamp = htonl(ts);
  header->ssrc = htonl(ssrc);
}

RTPHeader makeRTPHeaderFromPacket(uint8_t* packet) {
  RTPHeader header;
  memcpy(&header, packet, sizeof(RTPHeader));
  header.seq = ntohs(header.seq);
  header.timestamp = ntohl(header.timestamp);
  header.ssrc = ntohl(header.ssrc);
  return header;
}

void* dataReceiveThreadFn(void* arg) {
  uint8_t packet[RTP_HEADER_SIZE + DATA_SIZE];
  while (1) {
    socklen_t addr_len = sizeof(addr_recv);

    int recv_len =
        recvfrom(communication_sock, packet, RTP_HEADER_SIZE + DATA_SIZE, 0,
                 (struct sockaddr*)&addr_recv, &addr_len);
    if (recv_len < 0) {
      perror("recvfrom failed");
      continue;  // エラーが発生した場合は再試行
    }
    const int payload_len = recv_len - RTP_HEADER_SIZE;
    if (payload_len <= 0) {
      fprintf(stderr, "No data received or invalid packet size\n");
      continue;  // データなし or 不正
    }
    const RTPHeader header = makeRTPHeaderFromPacket(packet);
    const int port = header.ssrc;
    if (data_seq_latest[port - GATE_PORT] >= header.seq) {
      fprintf(stderr,
              "Received duplicate or out-of-order packet on port %d: "
              "expected seq %d, got %d\n",
              port, data_seq_latest[port - GATE_PORT], header.seq);
      continue;  // 重複または順序がずれたパケットは無視
    }
    // データをストア
    storeDataBuffer(port, packet + RTP_HEADER_SIZE, payload_len);
  }
}

void setup(const char* ip_addr, uint8_t hold) {
  // gateにアクセスして通信に使うポートを割り当ててもらう
  struct sockaddr_in gate_addr;
  const int gate_sock = setUpSocketTcp(&gate_addr, ip_addr, GATE_PORT);
  uint16_t port_u16;
  write(gate_sock, &hold, sizeof(hold));
  read(gate_sock, &port_u16, sizeof(port_u16));
  close(gate_sock);

  // UDPで通信ソケットをセットアップ
  communication_port = ntohs(port_u16);
  communication_sock = setUpSocketUdp(&addr_send, ip_addr, communication_port);

  // start data receive thread
  if (pthread_create(&data_receive_thread, NULL, dataReceiveThreadFn, NULL) !=
      0) {
    perror("Failed to create data receive thread");
    exit(EXIT_FAILURE);
  }
}

// RTP用グローバル変数
static uint16_t rtp_seq = 0;
static uint32_t rtp_timestamp = 0;

int receiveData(void* buf, int lens[], int ports[]) {
  int num_clients = 0;
  for (int port = GATE_PORT; port < MAX_PORT; port++) {
    int len = getDataBuffer(port, (uint8_t*)(buf + DATA_SIZE * num_clients));
    if (len > 0) {
      lens[num_clients] = len;
      ports[num_clients] = port;
      num_clients++;
    }
  }
  return num_clients;
}

int sendData(const void* buf, int len) {
  uint8_t packet[RTP_HEADER_SIZE + len];
  makeRTPHeader((RTPHeader*)packet, rtp_seq++, rtp_timestamp,
                communication_port);
  // 16bit PCMデータをリトル→ビッグエンディアン変換
  memcpy(packet + RTP_HEADER_SIZE, buf, len);
  const int sample_count = len / 2;
  uint16_t* pcm = (uint16_t*)(packet + RTP_HEADER_SIZE);
  for (int i = 0; i < sample_count; i++) {
    pcm[i] = htons(pcm[i]);
  }
  rtp_timestamp += len;  // サンプル数分進める（用途に応じて調整）
  return sendto(communication_sock, packet, RTP_HEADER_SIZE + len, 0,
                (struct sockaddr*)&addr_send, sizeof(addr_send));
}

void cleanUp() {
  if (communication_sock >= 0) {
    close(communication_sock);
  }
  if (data_receive_thread) {
    pthread_cancel(data_receive_thread);
    pthread_join(data_receive_thread, NULL);
  }
}

pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

// [ポート][リングバッファ][データ長1024]
static uint8_t data_buffer[MAX_PORT - GATE_PORT][RING_SIZE][DATA_SIZE];
static int data_lengths[MAX_PORT - GATE_PORT][RING_SIZE] = {{0}};
static int head[MAX_PORT - GATE_PORT] = {0};
static int tail[MAX_PORT - GATE_PORT] = {0};
static int count[MAX_PORT - GATE_PORT] = {0};

// 書き込み: ポート番号, データ, 長さ
void storeDataBuffer(int port, const uint8_t* data, int len) {
  // fprintf(stderr, "storeDataBuffer called for port %d, len = %d\n", port,
  // len);
  if (port < GATE_PORT || port >= MAX_PORT) return;
  int idx = port - GATE_PORT;
  pthread_mutex_lock(&data_mutex);
  if (count[idx] == RING_SIZE) {
    // 上書き（古いデータを捨てる）
    tail[idx] = (tail[idx] + 1) % RING_SIZE;
    count[idx]--;
    // fprintf(
    //     stderr,
    //     "Warning: Data buffer for port %d is full, overwriting oldest
    //     data\n", port);
  }
  memcpy(data_buffer[idx][head[idx]], data, len);
  data_lengths[idx][head[idx]] = len;
  head[idx] = (head[idx] + 1) % RING_SIZE;
  count[idx]++;
  // fprintf(stderr, "storeDataBuffer for port %d, count = %d\n", port,
  // count[idx]);
  pthread_mutex_unlock(&data_mutex);
}

// 読み出し: ポート番号, データ格納先, 戻り値=データ長（なければ0）
int getDataBuffer(int port, uint8_t* data) {
  // fprintf(stderr, "getDataBuffer called for port %d\n", port);
  if (port < GATE_PORT || port >= MAX_PORT) return 0;
  int idx = port - GATE_PORT;
  pthread_mutex_lock(&data_mutex);
  if (count[idx] == 0) {
    pthread_mutex_unlock(&data_mutex);
    return 0;
  }
  // fprintf(stderr, "getDataBuffer for port %d, count = %d\n", port,
  // count[idx]);
  int len = data_lengths[idx][tail[idx]];
  memcpy(data, data_buffer[idx][tail[idx]], len);
  // 16bit PCMデータをビッグ→リトルエンディアン変換
  const int sample_count = len / 2;
  uint16_t* pcm = (uint16_t*)data;
  for (int i = 0; i < sample_count; i++) {
    pcm[i] = ntohs(pcm[i]);
  }
  tail[idx] = (tail[idx] + 1) % RING_SIZE;
  count[idx]--;
  pthread_mutex_unlock(&data_mutex);
  return len;
}

// バッファ内のデータ数を取得
int getBufferCount(int port) {
  if (port < GATE_PORT || port >= MAX_PORT) return 0;
  int idx = port - GATE_PORT;
  pthread_mutex_lock(&data_mutex);
  int c = count[idx];
  pthread_mutex_unlock(&data_mutex);
  return c;
}