#pragma once

/// @return output length
int encode(const char* input, int input_len, char* output);

/// @return output length
int decode(const char* input, int input_len, char* output);