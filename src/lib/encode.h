#pragma once

#define SAMPLE_RATE 44100
#define MIN_FREQ 300
#define MAX_FREQ 3400

/// @return output length
int encode(const char* input, int input_len, char* output);

/// @return output length
int decode(const char* input, int input_len, char* output);