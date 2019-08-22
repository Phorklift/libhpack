#include "hpack.h"
#include "hpack_dynamic.h"

#include <stdlib.h>
#include <string.h>

/* Hooks */
void * (*hpack_index_alloc)(size_t) = malloc;
void (*hpack_index_free)(void *) = free;

void * (*hpack_entry_alloc)(size_t) = malloc;
void (*hpack_entry_free)(void *) = free;

struct hpack_dynamic_entry * (*hpack_entry_outside_get)(
		const char *name_str, int name_len,
		const char *value_str, int value_len) = NULL;
void (*hpack_entry_outside_unlink)(struct hpack_dynamic_entry *) = NULL;


#define HDENTRY_EXT_SIZE 32 /* RFC 7541 Section 4.1 */
#define HDINDEX_BEGIN 61


static int hpack_dynamic_index_increase(struct hpack *hpack)
{
#define INDEX_SIZE (sizeof(uintptr_t) * hpack->index_size)
	if (hpack->indexs == NULL) {
		hpack->index_size = 16;
		hpack->indexs = hpack_index_alloc(INDEX_SIZE);
		if (hpack->indexs == NULL) {
			return HPERR_NOMEM;
		}

	} else if (hpack->index_used == hpack->index_size) {
		/* double indexs if full */
		void *new_indexs = hpack_index_alloc(INDEX_SIZE * 2);
		if (new_indexs == NULL) {
			return HPERR_NOMEM;
		}

		memcpy(new_indexs, hpack->indexs, INDEX_SIZE);
		hpack->index_size *= 2;
		hpack_index_free(hpack->indexs);
		hpack->indexs = new_indexs;
	}

	hpack->index_used++;
	return 0;
}

int hpack_dynamic_entry_add(struct hpack *hpack,
		const char *name_str, int name_len,
		const char *value_str, int value_len)
{
	/* increase index */
	int buf_length = value_len + name_len + HDENTRY_EXT_SIZE;
	int ret = hpack_dynamic_table_size_adjust(hpack, buf_length);
	if (ret < 0) {
		return ret;
	}

	ret = hpack_dynamic_index_increase(hpack);
	if (ret < 0) {
		return ret;
	}

	/* add entry */
	uintptr_t flag_outside = 0;
	struct hpack_dynamic_entry *e;

	/* try user-defined outside */
	if (hpack_entry_outside_get != NULL) {
		e = hpack_entry_outside_get(name_str, name_len,
				value_str, value_len);
		if (e != NULL) {
			flag_outside = 1;
			goto out;
		}
	}

	/* create normal entry */
	e = hpack_entry_alloc(sizeof(struct hpack_dynamic_entry) + name_len + value_len);
	if (e == NULL) {
		return HPERR_NOMEM;
	}
	e->name_len = name_len;
	e->value_len = value_len;
	memcpy(e->str, name_str, name_len);
	memcpy(e->str + name_len, value_str, value_len);

out:
	hpack->indexs[hpack->index_used - 1] = ((uintptr_t)e) | flag_outside;
	hpack->buf_used += buf_length;
	return 0;
}

struct hpack_dynamic_entry *hpack_dynamic_entry_get(struct hpack *hpack, int index)
{
	if (index <= HDINDEX_BEGIN || index > hpack->index_used + HDINDEX_BEGIN) {
		return NULL;
	}

	uintptr_t e = hpack->indexs[hpack->index_used + HDINDEX_BEGIN - index];
	return (struct hpack_dynamic_entry *)(e & ~1UL);
}

static void hpack_dynamic_entry_free(uintptr_t index)
{
	struct hpack_dynamic_entry *e = (struct hpack_dynamic_entry *)(index & ~1UL);
	uintptr_t flag_outside = index & 1UL;

	if (flag_outside) {
		hpack_entry_outside_unlink(e);
	} else {
		hpack_entry_free(e);
	}
}

void hpack_free(struct hpack *hpack)
{
	int i;
	for (i = 0; i < hpack->index_used; i++) {
		hpack_dynamic_entry_free(hpack->indexs[i]);
	}
	hpack_index_free(hpack->indexs);
}

int hpack_dynamic_table_size_adjust(struct hpack *hpack, int length)
{
	if (length > hpack->buf_max) {
		return HPERR_DYN_ENTRY_TOO_LONG;
	}

	int n = 0;
	while (hpack->buf_used + length > hpack->buf_max) {
		uintptr_t index = hpack->indexs[n++];
		struct hpack_dynamic_entry *e = (struct hpack_dynamic_entry *)(index & ~1UL);
		hpack->buf_used -= e->name_len + e->value_len + HDENTRY_EXT_SIZE;

		hpack_dynamic_entry_free(index);
	}

	if (n != 0) {
		hpack->index_used -= n;
		memmove(hpack->indexs, hpack->indexs + n,
				sizeof(uintptr_t) * hpack->index_used);
	}

	return 0;
}
