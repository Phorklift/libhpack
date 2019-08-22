#include <stdio.h>
#include <string.h>
#include "hpack.h"
#include "huffman.h"
#include "hpack_dynamic.h"

#define HPACK_STATIC_TABLE_SIZE (sizeof(hpack_static_table) / sizeof(struct hpack_static_entry))
struct hpack_static_entry {
	const char	*name_str;
	const char	*value_str;
	int		name_len;
	int		value_len;
};

#define STATIC_TABLE_ENTRY(name, value)	{name, value, sizeof(name)-1, sizeof(value)-1}
static struct hpack_static_entry hpack_static_table[] = {
	STATIC_TABLE_ENTRY("", ""),
	STATIC_TABLE_ENTRY(":authority", ""),
	STATIC_TABLE_ENTRY(":method", "GET"),
	STATIC_TABLE_ENTRY(":method", "POST"),
	STATIC_TABLE_ENTRY(":path", "/"),
	STATIC_TABLE_ENTRY(":path", "/index.html"),
	STATIC_TABLE_ENTRY(":scheme", "http"),
	STATIC_TABLE_ENTRY(":scheme", "https"),
	STATIC_TABLE_ENTRY(":status", "200"),
	STATIC_TABLE_ENTRY(":status", "204"),
	STATIC_TABLE_ENTRY(":status", "206"),
	STATIC_TABLE_ENTRY(":status", "304"),
	STATIC_TABLE_ENTRY(":status", "400"),
	STATIC_TABLE_ENTRY(":status", "404"),
	STATIC_TABLE_ENTRY(":status", "500"),
	STATIC_TABLE_ENTRY("accept-charset", ""),
	STATIC_TABLE_ENTRY("accept-encoding", "gzip, deflate"),
	STATIC_TABLE_ENTRY("accept-language", ""),
	STATIC_TABLE_ENTRY("accept-ranges", ""),
	STATIC_TABLE_ENTRY("accept", ""),
	STATIC_TABLE_ENTRY("access-control-allow-origin", ""),
	STATIC_TABLE_ENTRY("age", ""),
	STATIC_TABLE_ENTRY("allow", ""),
	STATIC_TABLE_ENTRY("authorization", ""),
	STATIC_TABLE_ENTRY("cache-control", ""),
	STATIC_TABLE_ENTRY("content-disposition", ""),
	STATIC_TABLE_ENTRY("content-encoding", ""),
	STATIC_TABLE_ENTRY("content-language", ""),
	STATIC_TABLE_ENTRY("content-length", ""),
	STATIC_TABLE_ENTRY("content-location", ""),
	STATIC_TABLE_ENTRY("content-range", ""),
	STATIC_TABLE_ENTRY("content-type", ""),
	STATIC_TABLE_ENTRY("cookie", ""),
	STATIC_TABLE_ENTRY("date", ""),
	STATIC_TABLE_ENTRY("etag", ""),
	STATIC_TABLE_ENTRY("expect", ""),
	STATIC_TABLE_ENTRY("expires", ""),
	STATIC_TABLE_ENTRY("from", ""),
	STATIC_TABLE_ENTRY("host", ""),
	STATIC_TABLE_ENTRY("if-match", ""),
	STATIC_TABLE_ENTRY("if-modified-since", ""),
	STATIC_TABLE_ENTRY("if-none-match", ""),
	STATIC_TABLE_ENTRY("if-range", ""),
	STATIC_TABLE_ENTRY("if-unmodified-since", ""),
	STATIC_TABLE_ENTRY("last-modified", ""),
	STATIC_TABLE_ENTRY("link", ""),
	STATIC_TABLE_ENTRY("location", ""),
	STATIC_TABLE_ENTRY("max-forwards", ""),
	STATIC_TABLE_ENTRY("proxy-authenticate", ""),
	STATIC_TABLE_ENTRY("proxy-authorization", ""),
	STATIC_TABLE_ENTRY("range", ""),
	STATIC_TABLE_ENTRY("referer", ""),
	STATIC_TABLE_ENTRY("refresh", ""),
	STATIC_TABLE_ENTRY("retry-after", ""),
	STATIC_TABLE_ENTRY("server", ""),
	STATIC_TABLE_ENTRY("set-cookie", ""),
	STATIC_TABLE_ENTRY("strict-transport-security", ""),
	STATIC_TABLE_ENTRY("transfer-encoding", ""),
	STATIC_TABLE_ENTRY("user-agent", ""),
	STATIC_TABLE_ENTRY("vary", ""),
	STATIC_TABLE_ENTRY("via", ""),
	STATIC_TABLE_ENTRY("www-authenticate", ""),
};


int hpack_max_size(struct hpack *hpack, int max_size)
{
	if (max_size < 0) {
		return max_size;
	}
	hpack->buf_max = max_size;
	return hpack_dynamic_table_size_adjust(hpack, 0);
}

static int hpack_get(struct hpack *hpack, int index,
		const char **name_str, int *name_len,
		const char **value_str, int *value_len)
{
	if (index <= 0) {
		return index;
	}

	if (index < HPACK_STATIC_TABLE_SIZE) {
		struct hpack_static_entry *se = &hpack_static_table[index];
		*name_str = se->name_str;
		*name_len = se->name_len;
		if (value_str != NULL) {
			*value_str = se->value_str;
			*value_len = se->value_len;
		}
	} else {
		struct hpack_dynamic_entry *de = hpack_dynamic_entry_get(hpack, index);
		if (de == NULL) {
			return HPERR_INVALID_DYNAMIC_INDEX;
		}
		*name_str = de->str;
		*name_len = de->name_len;
		if (value_str != NULL) {
			*value_str = de->str + de->name_len;
			*value_len = de->value_len;
		}
	}

	return 0;
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

int hpack_decode_header(struct hpack *hpack,
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
		hpack_dynamic_entry_add(hpack, *name_str, *name_len, *value_str, *value_len);
	}

	return in_pos - in_buf;
}
