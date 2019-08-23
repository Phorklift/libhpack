#ifndef HPACK_H
#define HPACK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


/**
 * @brief Call this before other functions.
 *
 * If @dynamic_share=true, dynamic entries will be shared amount all hpacks,
 * which saves memory while costs a little more CPU.
 */
void hpack_library_init(bool dynamic_share);

/**
 * @brief The hpack context for dynamic encoding or decoding.
 */
typedef struct hpack_s hpack_t;

/**
 * @brief Create a hpack context.
 */
hpack_t *hpack_new(int max_size);

/**
 * @brief Free a hpack context.
 */
void hpack_free(hpack_t *);

/**
 * @brief Reset the max_size.
 *
 * Return 0 if OK, or negetive error code if fail.
 */
int hpack_max_size(hpack_t *hpack, int max_size);

/**
 * @brief Decode a header from buffer defined by @in_buf and @in_end,
 * to name-value pair.
 * The name-value buffers will be covered in next decoding, so you
 * must copy them out if you want use them later.
 *
 * Return processed input buffer length if OK, or negetive error code if fail.
 */
int hpack_decode_header(hpack_t *hpack,
		const uint8_t *in_buf, const uint8_t *in_end,
		const char **name_str, int *name_len,
		const char **value_str, int *value_len);

/**
 * @brief Encode :status header.
 *
 * This is a special case of hpack_encode_header().
 *
 * Since :status is in static-table and the value is an integer,
 * dynamic table will not be used. So hpack context argument is not need.
 *
 * Return encoded buffer length if OK, or negetive error code if fail.
 */
int hpack_encode_status(int status, uint8_t *out_buf, uint8_t *out_end);

/**
 * @brief Encode content_length header.
 *
 * This is a special case of hpack_encode_header().
 *
 * Since content_length is in static-table and the value is an integer,
 * dynamic table will not be used. So hpack context argument is not need.
 *
 * Return encoded buffer length if OK, or negetive error code if fail.
 */
int hpack_encode_content_length(int content_length, uint8_t *out_buf, uint8_t *out_end);

/**
 * @brief Encode a header.
 *
 * If you do not want add the header into dynamic table if not in static
 * table, you can pass "NULL" as the hpack argument.
 *
 * Return encoded buffer length if OK, or negetive error code if fail.
 */
int hpack_encode_header(hpack_t *hpack, const char *name_str, int name_len,
		const char *value_str, int value_len,
		uint8_t *out_buf, uint8_t *out_end);

/**
 * @brief Convert error code into string.
 */
const char *hpack_strerror(int hperr_no);

/**
 * @brief Error codes.
 */
enum HPACK_ERRNO {
	HPERR_NOMEM = -1000,
	HPERR_AGAIN,
	HPERR_NO_SPACE,
	HPERR_DECODE_INT,
	HPERR_HUFFMAN,
	HPERR_DYN_ENTRY_TOO_LONG,
	HPERR_INVALID_DYNAMIC_INDEX,
};

#endif
