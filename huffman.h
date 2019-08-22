#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>

int huffman_decode(const uint8_t *in_buf, int in_len, char *out_buf, int out_len);

#endif
