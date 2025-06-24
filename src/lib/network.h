#pragma once
#include <stdint.h>
/// @brief 受信データを取得する
/// @param buf 音声データを受けるポインタ. DATA_SIZE x (MAX_PORT -
/// GATE_PORT)のサイズ
/// @param lens 各ポートのデータ長を格納する配列.
/// @param ports ポート番号を格納する配列.
/// @return num_clients 受信したクライアント数
int receiveData(void* buf, int lens[], int ports[]);
int sendData(const void* buf, int len);
void cleanUp();
void setup(const char* ip_add, uint8_t hold);