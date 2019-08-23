#include <stdbool.h>
#include <string.h>

#include "wuy_dict.h"

#include "hpack_static.h"

#define HPACK_STATIC_TABLE_SIZE (sizeof(hpack_static_table) / sizeof(hpack_static_entry_t))
typedef struct {
	const char		*name_str;
	const char		*value_str;
	int			name_len;
	int			value_len;
	int			index;
	wuy_hlist_node_t	hash_node;
} hpack_static_entry_t;

#define STATIC_TABLE_ENTRY(name, value)	\
		{.name_str=name, .name_len=sizeof(name)-1, .value_str=value, .value_len=sizeof(value)-1}
static hpack_static_entry_t hpack_static_table[] = {
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


bool hpack_static_decode(int index, const char **name_str, int *name_len,
		const char **value_str, int *value_len)
{
	if (index < 1 || index >= HPACK_STATIC_TABLE_SIZE) {
		return false;
	}

	hpack_static_entry_t *se = &hpack_static_table[index];
	*name_str = se->name_str;
	*name_len = se->name_len;
	if (value_str != NULL) {
		*value_str = se->value_str;
		*value_len = se->value_len;
	}
	return true;
}

static wuy_dict_t *hpack_static_dict;
int hpack_static_encode_name(const char *name_str, int name_len)
{
	hpack_static_entry_t key = {
		.name_str = name_str,
		.name_len = name_len,
	};
	hpack_static_entry_t *se = wuy_dict_get(hpack_static_dict, &key);
	if (se == NULL) {
		return -1;
	}
	return se->index;
}

static uint32_t hpack_static_dict_hash(const void *item)
{
	const hpack_static_entry_t *se = item;
	return wuy_dict_hash_lenstr(se->name_str, se->name_len);
}
static bool hpack_static_dict_equal(const void *a, const void *b)
{
	const hpack_static_entry_t *sea = a;
	const hpack_static_entry_t *seb = b;
	return sea->name_len == seb->name_len && memcmp(sea->name_str, seb->name_str, sea->name_len) == 0;
}

void hpack_static_init(void)
{
	hpack_static_dict = wuy_dict_new_func(hpack_static_dict_hash,
			hpack_static_dict_equal,
			offsetof(hpack_static_entry_t, hash_node));

	wuy_dict_disable_expasion(hpack_static_dict, HPACK_STATIC_TABLE_SIZE * 2);

	int i;
	for (i = 1; i < HPACK_STATIC_TABLE_SIZE; i++) {
		hpack_static_entry_t *se = &hpack_static_table[i];
		if (wuy_dict_get(hpack_static_dict, se) != NULL) {
			continue;
		}
		se->index = i;
		wuy_dict_add(hpack_static_dict, se);
	}
}
