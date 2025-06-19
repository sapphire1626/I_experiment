#pragma once

void setupReceive(const char* ip_addr, int port, const char* protocol);
void setupSend(const char* ip_addr, int port, const char* protocol);
int receiveData(void* buf, int len);
int sendData(const void* buf, int len);
void cleanUp();