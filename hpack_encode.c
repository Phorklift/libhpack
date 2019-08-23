#include <stdio.h>
#include <string.h>

#include "hpack.h"
#include "huffman.h"
#include "hpack_static.h"
#include "hpack_dynamic.h"


int hpack_encode_status(int status, uint8_t *out_buf, uint8_t *out_end)
{
	int index;
	switch (status) {
	case 200:
		index = 8;
		break;
	case 204:
		index = 9;
		break;
	case 206:
		index = 10;
		break;
	case 304:
		index = 11;
		break;
	case 400:
		index = 12;
		break;
	case 404:
		index = 13;
		break;
	case 500:
		index = 14;
		break;
	default:
		{
		char str[10];
		int len = sprintf(str, "%d", status);
		return hpack_encode_header(NULL, ":status", 7,
				str, len, out_buf, out_end);
		}
	}

	if (out_end < out_buf + 1) {
		return HPERR_NO_SPACE;
	}
	out_buf[0] = 0x80 | index;
	return 1;
}

int hpack_encode_content_length(int content_length, uint8_t *out_buf, uint8_t *out_end)
{
	if (out_end < out_buf + 3) {
		return HPERR_NO_SPACE;
	}

	out_buf[0] = 0x40 | 28; /* index of content-length */

	int size = out_end - out_buf - 2;
	int len = snprintf((char *)out_buf + 2, size, "%d", content_length);
	if (len == size) {
		return HPERR_NO_SPACE;
	}
	out_buf[1] = len;
	return 2 + len;
}

static int hpack_encode_int(int n, uint8_t prefix_bits,
		uint8_t *out_buf, uint8_t *out_end)
{
	uint8_t prefix_max = (1 << prefix_bits) - 1;

	uint8_t *out_pos = out_buf;
	if (out_pos >= out_end) {
		return -1;
	}

	if (n < prefix_max) { /* 1-charactor case */
		out_buf[0] |= n;
		return 1;
	}

	*out_pos++ |= prefix_max;
	n -= prefix_max;
	while (n > 0x7F) {
		*out_pos++ = 0x80 | (n & 0x7F);
		n >>= 7;
		if (out_pos >= out_end) {
			return -1;
		}
	}
	*out_pos++ = n;

	return out_pos - out_buf;
}

static int hpack_encode_string(const char *s, int str_len,
		uint8_t *out_buf, uint8_t *out_end)
{
	out_buf[0] = 0;
	int encode_len = hpack_encode_int(str_len, 7, out_buf, out_end);
	if (encode_len < 0) {
		return -1;
	}

	uint8_t *out_pos = out_buf + encode_len;
	if (out_end - out_pos < str_len) {
		return -1;
	}
	memcpy(out_pos, s, str_len);
	out_pos += str_len;

	return out_pos - out_buf;
}

static void hpack_downcase(char *dest, const char *src, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		if (src[i] >= 'A' && src[i] <= 'Z') {
			dest[i] = src[i] | 0x20;
		}
	}
}

int hpack_encode_header(hpack_t *hpack, const char *name_raw, int name_len,
		const char *value_raw, int value_len,
		uint8_t *out_buf, uint8_t *out_end)
{
	char name_str[name_len];
	char value_str[value_len];
	hpack_downcase(name_str, name_raw, name_len);
	hpack_downcase(value_str, value_raw, value_len);

	uint8_t *out_pos = out_buf;
	out_pos[0] = 0;

	int len;

	/* name */
	int index = hpack_static_encode_name(name_str, name_len);
	if (index < 0) {
		out_pos++;
		len = hpack_encode_string(name_str, name_len, out_pos, out_end);
	} else {
		len = hpack_encode_int(index, 4, out_pos, out_end);
	}
	if (len < 0) {
		return len;
	}
	out_pos += len;

	/* value */
	// TODO use hpack !!!
	len = hpack_encode_string(value_str, value_len, out_pos, out_end);
	out_pos += len;

	return out_pos - out_buf;
}
