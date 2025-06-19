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

int s_recv = -1;
int s_send = -1;
struct sockaddr_in addr_recv;
struct sockaddr_in addr_send;

typedef struct {
  uint8_t vpxcc;
  uint8_t mpt;
  uint16_t seq;
  uint32_t timestamp;
  uint32_t ssrc;
} RTPHeader;

void build_rtp_header(uint8_t* packet, uint16_t seq, uint32_t ts,
                      uint32_t ssrc) {
  packet[0] = 0x80;  // Version 2, no padding, no extension, CC=0
  packet[1] = 0x00 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 |
              0x00;                        // M=0, PT=0 (PCMU)
  packet[1] |= 0x00 | 0x00 | 0x00 | 0x08;  // PT=8 (PCMU)

  packet[2] = seq >> 8;
  packet[3] = seq & 0xFF;

  packet[4] = ts >> 24;
  packet[5] = ts >> 16;
  packet[6] = ts >> 8;
  packet[7] = ts & 0xFF;

  packet[8] = ssrc >> 24;
  packet[9] = ssrc >> 16;
  packet[10] = ssrc >> 8;
  packet[11] = ssrc & 0xFF;
}

void setupReceive(const char* ip_addr, int port, const char* protocol) {
  socklen_t addr_len_recv = sizeof(struct sockaddr_in);
  if (strcmp(protocol, "tcp") == 0) {
    s_recv = setUpSocketTcpServer(&addr_recv, &addr_len_recv, port);
  } else if (strcmp(protocol, "udp") == 0) {
    s_recv = setUpSocketUdpServer(&addr_recv, &addr_len_recv, port);
  } else {
    fprintf(stderr, "Unsupported protocol: %s\n", protocol);
    exit(EXIT_FAILURE);
  }
}

void setupSend(const char* ip_addr, int port, const char* protocol) {
  socklen_t addr_len_send = sizeof(struct sockaddr_in);
  if (strcmp(protocol, "tcp") == 0) {
    s_send = setUpSocketTcp(&addr_send, ip_addr, port);
  } else if (strcmp(protocol, "udp") == 0) {
    s_send = setUpSocketUdp(&addr_send, ip_addr, port);
  } else {
    fprintf(stderr, "Unsupported protocol: %s\n", protocol);
    exit(EXIT_FAILURE);
  }
}

int receiveData(void* buf, int len) {
  // return read(s_recv, buf, len);
  socklen_t addr_len = sizeof(addr_recv);
  return recvfrom(s_recv, buf, len, 0, (struct sockaddr*)&addr_recv, &addr_len);
}

int sendData(const void* buf, int len) {
  // return write(s_send, buf, len);
  return sendto(s_send, buf, len, 0, (struct sockaddr*)&addr_send,
                sizeof(addr_send));
}

void cleanUp() {
  if (s_recv >= 0) {
    close(s_recv);
  }
  if (s_send >= 0) {
    close(s_send);
  }
}