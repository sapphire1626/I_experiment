#include "network.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "setup_socket.h"

#define RTP_HEADER_SIZE 12
#define RTP_PORT 5004
#define SAMPLE_RATE 8000
#define PACKET_SIZE 160

int communication_sock = -1;
struct sockaddr_in addr_recv;
struct sockaddr_in addr_send;

typedef struct {
  uint8_t vpxcc; // version(2), padding(0), extension(0), CSRC count(0)
  uint8_t mpt;   // marker(0), payload type(0)
  uint16_t seq;  // sequence number
  uint32_t timestamp; // timestamp
  uint32_t ssrc; // synchronization source identifier
} __attribute__((packed)) RTPHeader;

void makeRTPHeader(RTPHeader* header, uint16_t seq, uint32_t ts, uint32_t ssrc) {
  header->vpxcc = 0x80; // Version 2, no padding, no extension, CC=0
  header->mpt = 0x00;   // M=0, PT=0 (PCMU)
  header->seq = htons(seq);
  header->timestamp = htonl(ts);
  header->ssrc = htonl(ssrc);
}

void setup(const char* ip_addr, int port) {
  // gateにアクセスして通信に使うポートを割り当ててもらう
  struct sockaddr_in gate_addr;
  const int gate_sock = setUpSocketTcp(&gate_addr, ip_addr, port);
  uint16_t port_u16;
  read(gate_sock, &port_u16, sizeof(port_u16));
  close(gate_sock);

  // UDPで通信ソケットをセットアップ
  const int communication_port = ntohs(port_u16);
  communication_sock = setUpSocketUdp(&addr_send, ip_addr, communication_port);
}

// RTP用グローバル変数
static uint16_t rtp_seq = 0;
static uint32_t rtp_timestamp = 0;
static uint32_t rtp_ssrc = 0x12345678;  // 任意の値でOK

int receiveData(void* buf, int len) {
  uint8_t packet[RTP_HEADER_SIZE + len];
  socklen_t addr_len = sizeof(addr_recv);
  int recv_len = recvfrom(communication_sock, packet, RTP_HEADER_SIZE + len, 0, (struct sockaddr*)&addr_recv, &addr_len);
  if (recv_len <= RTP_HEADER_SIZE) {
    return 0; // データなし or 不正
  }
  int payload_len = recv_len - RTP_HEADER_SIZE;
  memcpy(buf, packet + RTP_HEADER_SIZE, payload_len);
  return payload_len;
}

int sendData(const void* buf, int len) {
  uint8_t packet[RTP_HEADER_SIZE + len];
  makeRTPHeader((RTPHeader*)packet, rtp_seq++, rtp_timestamp, rtp_ssrc);
  memcpy(packet + RTP_HEADER_SIZE, buf, len);
  rtp_timestamp += len;  // サンプル数分進める（用途に応じて調整）
  return sendto(communication_sock, packet, RTP_HEADER_SIZE + len, 0, (struct sockaddr*)&addr_send,
                sizeof(addr_send));
}

void cleanUp() {
  if (communication_sock >= 0) {
    close(communication_sock);
  }
}