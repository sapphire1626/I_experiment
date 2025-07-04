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

/// @brief bind, listenまでおこなう
/// @param addr
/// @param len addrのサイズのポインタ
/// @param port
/// @return socketのファイルディスクリプタを返す
int setUpSocketTcpServer(struct sockaddr_in* addr, socklen_t* len, int port);

/// @brief UDPサーバー用ソケットを作成しbindまでおこなう
/// @param addr
/// @param len addrのサイズのポインタ
/// @param port
/// @return socketのファイルディスクリプタを返す
int setUpSocketUdpServer(struct sockaddr_in* addr, socklen_t* len, int port);