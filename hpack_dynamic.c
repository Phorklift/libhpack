#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "hpack.h"
#include "hpack_dynamic.h"

#define HPACK_DYNAMIC_EXTRA_SIZE	32	/* see RFC 7541 Section 4.1 */
#define HPACK_DYNAMIC_INDEX_BEGIN	61	/* static table size */

typedef struct {
	short		name_len;
	short		value_len;
	char		data[0];
} hpack_dynamic_entry_t;

struct hpack_s {
	int		buf_max;
	int		buf_used;

	int		index_size;
	int		index_used;

	hpack_dynamic_entry_t	**indexs;
};

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

static int hpack_dynamic_table_size_adjust(hpack_t *hpack, int length)
{
	if (length > hpack->buf_max) {
		return HPERR_DYN_ENTRY_TOO_LONG;
	}

	int n = 0;
	while (hpack->buf_used + length > hpack->buf_max) {
		hpack_dynamic_entry_t *de = hpack->indexs[n++];
		hpack->buf_used -= de->name_len + de->value_len + HPACK_DYNAMIC_EXTRA_SIZE;
		free(de);
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
	hpack_dynamic_entry_t *de = malloc(sizeof(hpack_dynamic_entry_t)
			+ name_len + value_len);
	if (de == NULL) {
		return HPERR_NOMEM;
	}
	de->name_len = name_len;
	de->value_len = value_len;
	memcpy(de->data, name_str, name_len);
	memcpy(de->data + name_len, value_str, value_len);

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
	*name_str = de->data;
	*name_len = de->name_len;
	if (value_str != NULL) {
		*value_str = de->data + de->name_len;
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
		free(hpack->indexs[i]);
	}
	free(hpack->indexs);
	free(hpack);
}
