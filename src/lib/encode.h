#pragma once

#define SAMPLE_RATE 44100
#define MIN_FREQ 300
#define MAX_FREQ 3400

// /// @return output length
int encode(const char* input, int input_len, char* output);

// /// @return output length
int decode(const char* input, int input_len, char* output);

// bufs: n人分のbuf
// n: 人数
// len: 1人分のbuf_size
// output: 圧縮後buf
// 戻り値: 圧縮後buf_size
// int encode_n(const short** bufs, int n, int len, char* output);

// input: 圧縮後buf
// input_len: 圧縮後buf_size
// bufs: n人分のbuf
// n: 人数
// len: 1人分のbuf_size
// 戻り値: 再生する音声のbuf_size
// int decode_n(const char* input, int input_len, short** bufs, int n, int len);