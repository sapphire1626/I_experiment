#pragma once

int receiveData(void* buf, int len);
int sendData(const void* buf, int len);
void cleanUp();
void setup(const char* ip_addr, int port);