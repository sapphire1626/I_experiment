#include "encode.h"

#include <string.h>

int encode(const char* input, int input_len, char* output) {
  memcpy(output, input, input_len);
  int output_len = input_len;
  return output_len;
}

int decode(const char* input, int input_len, char* output) {
  memcpy(output, input, input_len);
  int output_len = input_len;
  return output_len;
}