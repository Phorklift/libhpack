#ifndef HPACK_STATIC_H
#define HPACK_STATIC_H

#include <stdbool.h>

bool hpack_static_decode(int index, const char **name_str, int *name_len,
		const char **value_str, int *value_len);

int hpack_static_encode_name(const char *name_str, int name_len);

void hpack_static_init(void);

#endif
