#ifndef HPACK_DYNAMIC_H
#define HPACK_DYNAMIC_H

#include <stdint.h>

int hpack_dynamic_table_size_adjust(struct hpack *hpack, int length);

int hpack_dynamic_entry_add(struct hpack *hpack,
		const char *name_str, int name_len,
		const char *value_str, int value_len);

struct hpack_dynamic_entry *hpack_dynamic_entry_get(
		struct hpack *hpack, int index);
#endif
