#pragma once

void setupReceive(const char* ip_addr, int port);
void setupSend(int port);
int receiveData(void* buf, int len);
int sendData(const void* buf, int len);
void cleanUp();