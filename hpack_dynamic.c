#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "wuy_dict.h"

#include "hpack.h"
#include "hpack_dynamic.h"

#define HPACK_DYNAMIC_EXTRA_SIZE	32	/* see RFC 7541 Section 4.1 */
#define HPACK_DYNAMIC_INDEX_BEGIN	61	/* static table size */

typedef struct {
	wuy_hlist_node_t	hash_node;
	int			refers;
	int			name_len;
	int			value_len;
	union {
		char		data[0];
		struct {
			const char *name_str;
			const char *value_str;
		} key; /* used only as key for dict-searching */
	} u;
} hpack_dynamic_entry_t;


struct hpack_s {
	int			buf_max;
	int			buf_used;

	int			index_size;
	int			index_used;

	hpack_dynamic_entry_t	**indexs;
};


/* We share dynamic entries amount all hpacks, which saves memory
 * while costs a little more CPU.
 * Call hpack_dynamic_enable_share() to enable this. */
static wuy_dict_t *hpack_dynamic_dict = NULL;

static void hpack_dynamic_entry_strings(const hpack_dynamic_entry_t *de,
		const char **name_str, const char **value_str)
{
	if (de->refers == -1) {
		*name_str = de->u.key.name_str;
		*value_str = de->u.key.value_str;
	} else {
		*name_str = de->u.data;
		*value_str = de->u.data + de->name_len;
	}
}
static uint32_t hpack_dynamic_dict_hash(const void *item)
{
	const hpack_dynamic_entry_t *de = item;
	const char *name_str, *value_str;
	hpack_dynamic_entry_strings(de, &name_str, &value_str);

	return wuy_dict_hash_lenstr(name_str, de->name_len) ^ wuy_dict_hash_lenstr(value_str, de->value_len);
}
static bool hpack_dynamic_dict_equal(const void *a, const void *b)
{
	const hpack_dynamic_entry_t *dea = a;
	const hpack_dynamic_entry_t *deb = b;
	const char *name_str_a, *value_str_a;
	const char *name_str_b, *value_str_b;
	hpack_dynamic_entry_strings(dea, &name_str_a, &value_str_a);
	hpack_dynamic_entry_strings(deb, &name_str_b, &value_str_b);

	return dea->name_len == deb->name_len && dea->value_len == dea->value_len
			&& memcmp(name_str_a, name_str_b, dea->name_len) == 0
			&& memcmp(value_str_a, value_str_b, dea->value_len) == 0;
}

static void hpack_dynamic_shared_add(hpack_dynamic_entry_t *de)
{
	if (hpack_dynamic_dict == NULL) {
		de->refers = 0;
		return;
	}

	de->refers = 1;
	wuy_dict_add(hpack_dynamic_dict, de);
}
static hpack_dynamic_entry_t *hpack_dynamic_shared_get(
		const char *name_str, int name_len,
		const char *value_str, int value_len)
{
	if (hpack_dynamic_dict == NULL) {
		return NULL;
	}

	hpack_dynamic_entry_t key = {
		.refers = -1,
		.name_len = name_len,
		.value_len = value_len,
		.u.key.name_str = name_str,
		.u.key.value_str = value_str,
	};
	return wuy_dict_get(hpack_dynamic_dict, &key);
}
static bool hpack_dynamic_shared_unlink(hpack_dynamic_entry_t *de)
{
	if (hpack_dynamic_dict == NULL) {
		return true;
	}

	de->refers--;
	if (de->refers != 0) {
		return false;
	}
	wuy_dict_delete(hpack_dynamic_dict, de);
	return true;
}

static void hpack_dynamic_enable_share(void)
{
	hpack_dynamic_dict = wuy_dict_new_func(hpack_dynamic_dict_hash,
			hpack_dynamic_dict_equal,
			offsetof(hpack_dynamic_entry_t, hash_node));
}

/* OK, the shared dictory ends. The following is about hpack. */
static int hpack_dynamic_index_increase(hpack_t *hpack)
{
	if (hpack->indexs == NULL || hpack->index_used == hpack->index_size) {
		hpack->index_size *= 2;
		hpack->indexs = realloc(hpack->indexs,
				sizeof(hpack_dynamic_entry_t *) * hpack->index_size);
		if (hpack->indexs == NULL) {
			return HPERR_NOMEM;
		}
	}

	hpack->index_used++;
	return 0;
}

static void hpack_dynamic_entry_free(hpack_dynamic_entry_t *de)
{
	if (hpack_dynamic_shared_unlink(de)) {
		free(de);
	}
}

static int hpack_dynamic_table_size_adjust(hpack_t *hpack, int length)
{
	if (length > hpack->buf_max) {
		return HPERR_DYN_ENTRY_TOO_LONG;
	}

	int n = 0;
	while (hpack->buf_used + length > hpack->buf_max) {
		hpack_dynamic_entry_t *de = hpack->indexs[n++];
		hpack->buf_used -= de->name_len + de->value_len + HPACK_DYNAMIC_EXTRA_SIZE;
		hpack_dynamic_entry_free(de);
	}

	if (n != 0) {
		hpack->index_used -= n;
		memmove(hpack->indexs, hpack->indexs + n,
				sizeof(hpack_dynamic_entry_t *) * hpack->index_used);
	}

	return 0;
}

int hpack_max_size(hpack_t *hpack, int max_size)
{
	if (max_size < 0) {
		return max_size;
	}
	hpack->buf_max = max_size;
	return hpack_dynamic_table_size_adjust(hpack, 0);
}

int hpack_dynamic_add(hpack_t *hpack, const char *name_str, int name_len,
		const char *value_str, int value_len)
{
	/* increase index */
	int buf_length = value_len + name_len + HPACK_DYNAMIC_EXTRA_SIZE;
	int ret = hpack_dynamic_table_size_adjust(hpack, buf_length);
	if (ret < 0) {
		return ret;
	}

	ret = hpack_dynamic_index_increase(hpack);
	if (ret < 0) {
		return ret;
	}

	/* add entry */
	hpack_dynamic_entry_t *de = hpack_dynamic_shared_get(name_str,
			name_len, value_str, value_len);

	if (de == NULL) {
		de = malloc(sizeof(hpack_dynamic_entry_t) + name_len + value_len);
		if (de == NULL) {
			return HPERR_NOMEM;
		}
		de->name_len = name_len;
		de->value_len = value_len;
		memcpy(de->u.data, name_str, name_len);
		memcpy(de->u.data + name_len, value_str, value_len);
		hpack_dynamic_shared_add(de);
	}

	hpack->indexs[hpack->index_used - 1] = de;
	hpack->buf_used += buf_length;
	return 0;
}

bool hpack_dynamic_decode(hpack_t *hpack, int index,
		const char **name_str, int *name_len,
		const char **value_str, int *value_len)
{
	if (index <= HPACK_DYNAMIC_INDEX_BEGIN || index > hpack->index_used + HPACK_DYNAMIC_INDEX_BEGIN) {
		return false;
	}

	hpack_dynamic_entry_t *de = hpack->indexs[hpack->index_used + HPACK_DYNAMIC_INDEX_BEGIN - index];
	*name_str = de->u.data;
	*name_len = de->name_len;
	if (value_str != NULL) {
		*value_str = de->u.data + de->name_len;
		*value_len = de->value_len;
	}
	return true;
}


hpack_t *hpack_new(int buf_max)
{
	hpack_t *hpack = malloc(sizeof(hpack_t));
	if (hpack == NULL) {
		return NULL;
	}

	bzero(hpack, sizeof(hpack_t));
	hpack->buf_max = buf_max;
	hpack->index_size = 16;
	return hpack;
}

void hpack_free(hpack_t *hpack)
{
	int i;
	for (i = 0; i < hpack->index_used; i++) {
		hpack_dynamic_entry_free(hpack->indexs[i]);
	}
	free(hpack->indexs);
	free(hpack);
}

void hpack_dynamic_init(bool shared)
{
	if (shared) {
		hpack_dynamic_enable_share();
	}
}
