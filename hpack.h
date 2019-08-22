#ifndef HPACK_H
#define HPACK_H

#include <stdint.h>
#include <stddef.h>

/* This definition is placed here only for size-declaration.
 * Appaliations should not access the members directly. */
struct hpack {
	int		buf_max;
	int		buf_used;

	int		index_size;
	int		index_used;

	/* uintptr_t = (struct hpack_dynamic_entry *) | flag_outside */
	uintptr_t	*indexs;
};

void hpack_init(struct hpack *);
void hpack_free(struct hpack *);

int hpack_max_size(struct hpack *hpack, int max_size);

int hpack_decode_header(struct hpack *hpack,
		const uint8_t *in_buf, const uint8_t *in_end,
		const char **name_str, int *name_len,
		const char **value_str, int *value_len);

int hpack_encode_status(struct hpack *hpack, int status,
		uint8_t *out_buf, uint8_t *out_end);
int hpack_encode_content_length(struct hpack *hpack, int content_length,
		uint8_t *out_buf, uint8_t *out_end);
int hpack_encode_header(struct hpack *hpack, const char *name_str, int name_len,
		const char *value_str, int value_len,
		uint8_t *out_buf, uint8_t *out_end);

/* Only used by @hpack_entry_outside_get() hook. */
struct hpack_dynamic_entry {
	short		name_len;
	short		value_len;
	char		str[0];
};

/* Hooks */
extern void *(*hpack_index_alloc)(size_t);
extern void (*hpack_index_free)(void *);

extern void *(*hpack_entry_alloc)(size_t);
extern void (*hpack_entry_free)(void *);

extern struct hpack_dynamic_entry *(*hpack_entry_outside_get)(
		const char *name_str, int name_len,
		const char *value_str, int value_len);
extern void (*hpack_entry_outside_unlink)(struct hpack_dynamic_entry *);


const char *hpack_strerror(int hperr_no);
enum HPACK_ERRNO {
	HPERR_NOMEM = -1000,
	HPERR_AGAIN,
	HPERR_DECODE_INT,
	HPERR_HUFFMAN,
	HPERR_DYN_ENTRY_TOO_LONG,
	HPERR_INVALID_DYNAMIC_INDEX,
};

#endif
