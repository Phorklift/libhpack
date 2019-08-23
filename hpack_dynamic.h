#ifndef HPACK_DYNAMIC_H
#define HPACK_DYNAMIC_H

#include <stdbool.h>

#include "hpack.h"

int hpack_dynamic_add(hpack_t *hpack, const char *name_str, int name_len,
		const char *value_str, int value_len);

bool hpack_dynamic_decode(hpack_t *hpack, int index,
		const char **name_str, int *name_len,
		const char **value_str, int *value_len);

void hpack_dynamic_init(bool shared);

#endif
