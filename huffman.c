#include <stdio.h>

#include "huffman_table.h"
#include "huffman.h"

static char *huffdecode4(char *dst, uint8_t in, uint8_t *state, int *maybe_eos)
{
	const nghttp2_huff_decode *entry = huff_decode_table[*state] + in;

	if ((entry->flags & NGHTTP2_HUFF_FAIL) != 0)
		return NULL;
	if ((entry->flags & NGHTTP2_HUFF_SYM) != 0)
		*dst++ = entry->sym;
	*state = entry->state;
	*maybe_eos = (entry->flags & NGHTTP2_HUFF_ACCEPTED) != 0;

	return dst;
}

int huffman_decode(const uint8_t *in_buf, int in_len, char *out_buf, int out_len)
{
	char *out_pos = out_buf;
	uint8_t state = 0;
	int maybe_eos = 1;

	int i;
	for (i = 0; i < in_len; i++) {
		uint8_t ch = in_buf[i];
		if ((out_pos = huffdecode4(out_pos, ch >> 4, &state, &maybe_eos)) == NULL)
			return -1;
		if ((out_pos = huffdecode4(out_pos, ch & 0xf, &state, &maybe_eos)) == NULL)
			return -1;

		if (out_pos - out_buf >= out_len) {
			return -1;
		}
	}

	if (!maybe_eos)
		return -1;

	*out_pos = '\0';
	return out_pos - out_buf;
}
