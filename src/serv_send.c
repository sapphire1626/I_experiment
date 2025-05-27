#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "lib/setup_socket.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return 1;
  }

  const int port = atoi(argv[1]);

  struct sockaddr_in addr;
  socklen_t len = sizeof(struct sockaddr_in);
  int s = setUpSocketTcpServer(&addr, &len, port);

  char buf[256];
  int c;

  while ((c = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
    if (write(s, buf, c) < 0) {
      perror("write");
      return 1;
    }
  }
  if (c < 0) {
    perror("read");
    return 1;
  }

  close(s);

  return 0;
}