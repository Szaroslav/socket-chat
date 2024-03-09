#include "byte.h"
#include <string.h>

static const int INT_SIZE_BYTES = sizeof(int);

void int_to_bytes(byte *destination, int source) {
  unsigned int value = (unsigned int) source;
  memset(destination, 0, INT_SIZE_BYTES);

  int i = INT_SIZE_BYTES - 1;
  while (value) {
    destination[i--] = value & ONE_BYTE_MASK;
    value >>= BYTE_BITS;
  }
}

int bytes_to_int(const byte *source) {
  int result = 0;
  for (int i = 0; i < INT_SIZE_BYTES; i++) {
    result |= source[i] << (BYTE_BITS * (INT_SIZE_BYTES - (i + 1)));
  }
  return result;
}
