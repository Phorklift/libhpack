#include "hpack_static.h"
#include "hpack_dynamic.h"

#include "hpack.h"

const char *hpack_strerror(int errcode)
{
	switch (errcode) {
	case HPERR_NOMEM:
		return "not enough memory";
	case HPERR_AGAIN:
		return "not enough input";
	case HPERR_NO_SPACE:
		return "not enough output";
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
