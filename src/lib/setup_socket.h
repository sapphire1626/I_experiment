#pragma once
#include <netinet/in.h>
/// @brief
/// @param addr
/// @param ip_addr
/// @param port
/// @return socketのファイルディスクリプタを返す
int setUpSocketTcp(struct sockaddr_in* addr, const char* ip_addr, int port);

int setUpSocketUdp(struct sockaddr_in* addr, const char* ip_addr, int port);

void setUpSockaddr(struct sockaddr_in* addr, const char* ip_addr, int port);