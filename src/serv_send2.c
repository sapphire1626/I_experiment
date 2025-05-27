#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "lib/setup_socket.h"

FILE *fp;
int s;

// シグナルハンドラ（受信したシグナル番号を表示）
void sigint_handler(int sig) {
  //   fprintf(stderr, "Signal %d を受信しました。\n", sig);
  (void)pclose(fp);
  close(s);
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return 1;
  }
  // シグナルハンドラの設定
  signal(SIGINT, sigint_handler);

  const int port = atoi(argv[1]);

  struct sockaddr_in addr;
  socklen_t len = sizeof(struct sockaddr_in);
  s = setUpSocketTcpServer(&addr, &len, port);

  char buf[256];

  char *cmdline = "rec -t raw -b 16 -c 1 -e s -r 44100 -";
  if ((fp = popen(cmdline, "r")) == NULL) {
    perror("can not exec commad");
    exit(EXIT_FAILURE);
  }

  while (fread(buf, sizeof(buf[0]), sizeof(buf) / sizeof(buf[0]), fp) > 0) {
    if (write(s, buf, sizeof(buf)) < 0) {
      perror("write");
      pclose(fp);
      close(s);
      return 1;
    }
  }
  if (ferror(fp)) {
    perror("read");
    pclose(fp);
    close(s);
    return 1;
  }

  (void)pclose(fp);
  close(s);

  return 0;
}