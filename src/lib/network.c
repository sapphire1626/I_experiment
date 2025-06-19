#include "network.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "setup_socket.h"

int s_recv = -1;
int s_send = -1;
struct sockaddr_in addr_recv;
struct sockaddr_in addr_send;

void setupReceive(const char* ip_addr, int port) {
  s_recv = setUpSocketTcp(&addr_recv, ip_addr, port);
}

void setupSend(int port) {
  socklen_t addr_len_send = sizeof(struct sockaddr_in);
  s_send = setUpSocketTcpServer(&addr_send, &addr_len_send, port);
}

int receiveData(void* buf, int len) {
  return read(s_recv, buf, len);
}

int sendData(const void* buf, int len) {
  return write(s_send, buf, len);
}

void cleanUp() {
  if (s_recv >= 0) {
    close(s_recv);
  }
  if (s_send >= 0) {
    close(s_send);
  }
}