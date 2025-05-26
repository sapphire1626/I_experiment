#include "setup_socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int setUpSocketTcp(struct sockaddr_in* addr, const char* ip_addr, int port) {
  int s = socket(PF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    perror("socket");
    exit(1);
  }

  setUpSockaddr(addr, ip_addr, port);

  printf("connecting...\n");
  int ret = connect(s, (struct sockaddr*)addr, sizeof(*addr));
  if (ret < 0) {
    perror("connect");
    exit(1);
  }
  printf("connected\n");

  return s;
}

int setUpSocketUdp(struct sockaddr_in* addr, const char* ip_addr, int port) {
  int s = socket(PF_INET, SOCK_DGRAM, 0);
  if (s < 0) {
    perror("socket");
    exit(1);
  }

  setUpSockaddr(addr, ip_addr, port);

  return s;
}

void setUpSockaddr(struct sockaddr_in* addr, const char* ip_addr, int port) {
  addr->sin_family = AF_INET;  // IPv4
  if (inet_aton(ip_addr, &addr->sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }

  addr->sin_port = htons(port);
}