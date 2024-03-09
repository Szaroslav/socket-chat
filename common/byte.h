#ifndef __BYTE_H__
#define __BYTE_H__

#define BYTE_BITS     8
#define ONE_BYTE_MASK 0xff

typedef unsigned char byte;

void int_to_bytes(byte *source, int value);
int bytes_to_int(const byte *source);

#endif // __BYTE_H__
