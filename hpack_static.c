#include <stdbool.h>
#include <string.h>

#include "hpack_static.h"

struct hpack_static_entry {
	const char	*name_str;
	int		name_len;
	const char	*value_str;
	int		value_len;
};

#define STE_NAME_ONLY(name)		{ name, sizeof(name)-1 }
#define STE_NAME_VALUE(name, value)	{ name, sizeof(name)-1, value, sizeof(value)-1 }

static struct hpack_static_entry hpack_static_table[] =
{
	STE_NAME_ONLY(""),
	STE_NAME_ONLY(":authority"),
	STE_NAME_VALUE(":method", "GET"),
	STE_NAME_VALUE(":method", "POST"),
	STE_NAME_VALUE(":path", "/"),
	STE_NAME_VALUE(":path", "/index.html"),
	STE_NAME_VALUE(":scheme", "http"),
	STE_NAME_VALUE(":scheme", "https"),
	STE_NAME_VALUE(":status", "200"),
	STE_NAME_VALUE(":status", "204"),
	STE_NAME_VALUE(":status", "206"),
	STE_NAME_VALUE(":status", "304"),
	STE_NAME_VALUE(":status", "400"),
	STE_NAME_VALUE(":status", "404"),
	STE_NAME_VALUE(":status", "500"),
	STE_NAME_ONLY("accept-charset"),
	STE_NAME_VALUE("accept-encoding", "gzip, deflate"),
	STE_NAME_ONLY("accept-language"),
	STE_NAME_ONLY("accept-ranges"),
	STE_NAME_ONLY("accept"),
	STE_NAME_ONLY("access-control-allow-origin"),
	STE_NAME_ONLY("age"),
	STE_NAME_ONLY("allow"),
	STE_NAME_ONLY("authorization"),
	STE_NAME_ONLY("cache-control"),
	STE_NAME_ONLY("content-disposition"),
	STE_NAME_ONLY("content-encoding"),
	STE_NAME_ONLY("content-language"),
	STE_NAME_ONLY("content-length"),
	STE_NAME_ONLY("content-location"),
	STE_NAME_ONLY("content-range"),
	STE_NAME_ONLY("content-type"),
	STE_NAME_ONLY("cookie"),
	STE_NAME_ONLY("date"),
	STE_NAME_ONLY("etag"),
	STE_NAME_ONLY("expect"),
	STE_NAME_ONLY("expires"),
	STE_NAME_ONLY("from"),
	STE_NAME_ONLY("host"),
	STE_NAME_ONLY("if-match"),
	STE_NAME_ONLY("if-modified-since"),
	STE_NAME_ONLY("if-none-match"),
	STE_NAME_ONLY("if-range"),
	STE_NAME_ONLY("if-unmodified-since"),
	STE_NAME_ONLY("last-modified"),
	STE_NAME_ONLY("link"),
	STE_NAME_ONLY("location"),
	STE_NAME_ONLY("max-forwards"),
	STE_NAME_ONLY("proxy-authenticate"),
	STE_NAME_ONLY("proxy-authorization"),
	STE_NAME_ONLY("range"),
	STE_NAME_ONLY("referer"),
	STE_NAME_ONLY("refresh"),
	STE_NAME_ONLY("retry-after"),
	STE_NAME_ONLY("server"),
	STE_NAME_ONLY("set-cookie"),
	STE_NAME_ONLY("strict-transport-security"),
	STE_NAME_ONLY("transfer-encoding"),
	STE_NAME_ONLY("user-agent"),
	STE_NAME_ONLY("vary"),
	STE_NAME_ONLY("via"),
	STE_NAME_ONLY("www-authenticate"),
};

#define HPACK_STATIC_TABLE_SIZE (sizeof(hpack_static_table) / sizeof(struct hpack_static_entry))

bool hpack_static_decode(int index, const char **name_str, int *name_len,
		const char **value_str, int *value_len)
{
	if (index < 1 || index >= HPACK_STATIC_TABLE_SIZE) {
		return false;
	}

	struct hpack_static_entry *se = &hpack_static_table[index];
	*name_str = se->name_str;
	*name_len = se->name_len;
	if (value_str != NULL) {
		*value_str = se->value_str;
		*value_len = se->value_len;
	}
	return true;
}

/* a pre-defined perfect hash for hpack_static_table */
static int hpack_static_hash_buckets[244] = {
	0,0,0,0,0,0,0,0,0,0,0,1,0,18,0,0,0,0,42,0,61,0,0,0,0,48,0,40,0,43,0,0,
	0,0,0,0,0,0,0,0,0,22,0,51,0,0,50,53,0,59,0,33,0,0,0,0,0,0,0,0,0,0,0,0,
	0,44,0,0,0,0,0,28,6,0,0,36,60,0,0,0,0,0,0,0,0,0,0,0,0,0,23,0,0,0,0,0,
	0,0,0,0,0,0,0,0,20,0,24,0,0,0,57,0,0,0,0,0,0,0,0,0,0,21,0,0,0,0,0,29,
	0,0,25,0,38,17,0,0,0,19,0,0,0,56,0,0,0,15,0,0,0,0,46,16,0,0,0,0,0,37,
	32,0,0,0,0,0,31,30,0,39,27,0,0,0,41,0,0,0,0,0,0,0,0,54,0,0,0,2,26,0,0,
	0,55,0,0,0,0,0,0,52,8,0,0,0,0,0,0,35,0,0,34,0,0,0,0,0,0,0,47,0,0,4,0,
	0,0,0,0,0,0,0,0,49,0,0,0,0,0,0,0,58,0,0,0,45,0,0,
};
static int hpack_static_hash(const char *str, int len)
{
	return (str[0]*29*131 + str[len-1]*131 + len) % 244;
}

int hpack_static_encode_name(const char *name_str, int name_len)
{
	int hash = hpack_static_hash(name_str, name_len);
	int index = hpack_static_hash_buckets[hash];
	if (index == 0) {
		return -1;
	}
	struct hpack_static_entry *e = &hpack_static_table[index];
	return name_len == e->name_len && memcmp(e->name_str, name_str, name_len) == 0 ? index : -1;
}
