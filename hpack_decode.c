#include <stdio.h>
#include <string.h>

#include "hpack.h"
#include "huffman.h"
#include "hpack_static.h"
#include "hpack_dynamic.h"

static int hpack_get(hpack_t *hpack, int index,
		const char **name_str, int *name_len,
		const char **value_str, int *value_len)
{
	if (index <= 0) {
		return index;
	}

	if (hpack_static_decode(index, name_str, name_len, value_str, value_len)) {
		return 0;
	}

	if (hpack_dynamic_decode(hpack, index, name_str, name_len, value_str, value_len)) {
		return 0;
	}

	return HPERR_INVALID_DYNAMIC_INDEX;
}

static int hpack_decode_int(uint8_t const **in_pos_p, const uint8_t *in_end,
		int prefix_bits)
{
	uint8_t prefix_max = (1 << prefix_bits) - 1;

	uint8_t value = *((*in_pos_p)++);
	int n = value & prefix_max;
	if (n != prefix_max) {
		return n;
	}

	if (in_end - *in_pos_p > 3) { /* 3 bytes at most */
		in_end = *in_pos_p + 3;
	}

	int shift = 0;
	while (*in_pos_p < in_end) {
		value = *((*in_pos_p)++);
		n += (value & 0x7f) << shift;
		if ((value & 0x80) == 0) {
			return n;
		}
		shift += 7;
	}
	return HPERR_DECODE_INT;
}

static int hpack_decode_string(const uint8_t **in_pos_p, const uint8_t *in_end,
		const char **out_pos_p, int *out_len_p)
{
	int is_huffman = *in_pos_p[0] & 0x80;

	int len = hpack_decode_int(in_pos_p, in_end, 7);
	if (len < 0) {
		return len;
	}
	if (len > (in_end - *in_pos_p)) {
		return HPERR_AGAIN;
	}

	static char huffman_bufs[2][4096];
	static int huffman_count;
	if (is_huffman) {
		char *out = huffman_bufs[huffman_count++ % 2];
		int decode_len = huffman_decode(*in_pos_p, len, out, 4096);
		if (decode_len < 0) {
			return HPERR_HUFFMAN;
		}
		*in_pos_p += len;

		*out_pos_p = out;
		*out_len_p = decode_len;
		return 0;
	} else {
		*out_pos_p = (const char *)(*in_pos_p);
		*out_len_p = len;

		*in_pos_p += len;
		return 0;
	}
}

int hpack_decode_header(hpack_t *hpack,
		const uint8_t *in_buf, const uint8_t *in_end,
		const char **name_str, int *name_len,
		const char **value_str, int *value_len)
{
	if (in_buf >= in_end) {
		return HPERR_AGAIN;
	}

	uint8_t first = in_buf[0];
	const uint8_t *in_pos = in_buf;
	const uint8_t **in_pos_p = &in_pos;

	/* -- Indexed Header Field */
	if (first & 0x80) {
		int ret = hpack_get(hpack, hpack_decode_int(in_pos_p, in_end, 7),
				name_str, name_len, value_str, value_len);
		if (ret < 0) {
			return ret;
		}
		return in_pos - in_buf;
	}

	/* -- Literal Header Field */
	int prefix_bits;
	if (first & 0x40) { /* with Incremental Indexing */
		prefix_bits = 6;

	/* -- Dynamic Table Size Update */
	} else if (first & 0x20) {
		int ret = hpack_max_size(hpack, hpack_decode_int(in_pos_p, in_end, 4));
		if (ret < 0) {
			return ret;
		}
		return in_pos - in_buf;

	} else if (first & 0x10) { /* Never Indexed */
		prefix_bits = 4;
	} else { /* without Indexing */
		prefix_bits = 4;
	}

	/* name */
	int index = hpack_decode_int(in_pos_p, in_end, prefix_bits);
	int ret = (index == 0)
			? hpack_decode_string(in_pos_p, in_end, name_str, name_len)
			: hpack_get(hpack, index, name_str, name_len, NULL, NULL);
	if (ret < 0) {
		return ret;
	}

	/* value */
	ret = hpack_decode_string(in_pos_p, in_end, value_str, value_len);
	if (ret < 0) {
		return ret;
	}

	/* add to dynamic table */
	if (prefix_bits == 6) {
		hpack_dynamic_add(hpack, *name_str, *name_len, *value_str, *value_len);
	}

	return in_pos - in_buf;
}
