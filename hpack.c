#include <stdlib.h>
#include "hpack.h"

void hpack_init(struct hpack *hpack)
{
	hpack->buf_max = 4096; /* RFC7540 section 6.5.2 */
	hpack->buf_used = 0;

	hpack->index_used = 0;
	hpack->index_size = 0;
	hpack->indexs = NULL;
}

const char *hpack_strerror(int errcode)
{
	switch (errcode) {
	case HPERR_NOMEM:
		return "not enough memory";
	case HPERR_AGAIN:
		return "not enough input";
	case HPERR_DECODE_INT:
		return "invalid decode int";
	case HPERR_HUFFMAN:
		return "invalid decode huffman";
	case HPERR_DYN_ENTRY_TOO_LONG:
		return "too long push entry";
	case HPERR_INVALID_DYNAMIC_INDEX:
		return "invalid dynamic index";
	default:
		return errcode < 0 ? "invalid error code" : "OK";
	}
}
