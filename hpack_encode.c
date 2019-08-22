#include <stdio.h>
#include <string.h>
#include "hpack.h"
#include "huffman.h"

static char *static_table[] = {
	// "accept-charset",
	// ("accept-encoding: gzip, deflate", 15),
	"accept-language",
	"accept-ranges",
	"accept",
	"access-control-allow-origin",
	"age",
	"allow",
	"authorization",
	"cache-control",
	"content-disposition",
	"content-encoding",
	"content-language",
	"content-length",
	"content-location",
	"content-range",
	"content-type",
	"cookie",
	"date",
	"etag",
	"expect",
	"expires",
	"from",
	"host",
	"if-match",
	"if-modified-since",
	"if-none-match",
	"if-range",
	"if-unmodified-since",
	"last-modified",
	"link",
	"location",
	"max-forwards",
	"proxy-authenticate",
	"proxy-authorization",
	"range",
	"referer",
	"refresh",
	"retry-after",
	"server",
	"set-cookie",
	"strict-transport-security",
	"transfer-encoding",
	"user-agent",
	"vary",
	"via",
	"www-authenticate",
};


int hpack_encode_status(struct hpack *hpack, int status, uint8_t *out_buf, uint8_t *out_end)
{
	char str[10];
	int len, index;
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
		len = sprintf(str, "%d", status);
		return hpack_encode_header(hpack, ":status", 7,
				str, len, out_buf, out_end);
	}

	if (out_end == out_buf) {
		return -1;
	}
	out_buf[0] = 0x80 | index;
	return 1;
}

int hpack_encode_content_length(struct hpack *hpack, int content_length,
		uint8_t *out_buf, uint8_t *out_end)
{
	if (out_end - out_buf < 2 + 20) {
		return -1;
	}
	out_buf[0] = 0x40 | 28; /* index of content-length */
	out_buf[1] = sprintf((char *)out_buf + 2, "%d", content_length);
	return 2 + out_buf[1];
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

int hpack_encode_header(struct hpack *hpack, const char *name_raw, int name_len,
		const char *value_raw, int value_len,
		uint8_t *out_buf, uint8_t *out_end)
{
	char name_str[name_len];
	char value_str[value_len];
	hpack_downcase(name_str, name_raw, name_len);
	hpack_downcase(value_str, value_raw, value_len);

	int index = -1;
	int i;
	for (i = 0; i < sizeof(static_table) / sizeof(char *); i++) {
		if (memcmp(name_str, static_table[i], name_len) == 0) {
			index = i;
			break;
		}
	}

	uint8_t *out_pos = out_buf;
	int len;
	out_pos[0] = 0;
	if (index == -1) {
		out_pos++;
		len = hpack_encode_string(name_str, name_len, out_pos, out_end);
	} else {
		len = hpack_encode_int(index + 17, 4, out_pos, out_end);
	}
	if (len < 0) {
		return -1;
	}
	out_pos += len;

	len = hpack_encode_string(value_str, value_len, out_pos, out_end);
	out_pos += len;

	return out_pos - out_buf;
}
