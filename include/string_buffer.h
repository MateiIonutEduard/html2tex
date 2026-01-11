#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct StringBuffer {
		char* data;
		size_t length;
		size_t capacity;
		int error;
	} StringBuffer;

	/**
	 * @brief Creates a new string buffer with specified initial capacity.
	 * @param initial_capacity Initial buffer size in bytes (excluding null terminator)
	 * @param 0 Uses default (64 bytes)
	 * @param >0 Requested capacity (clamped to system limits)
	 * @return Success: Initialized StringBuffer* (caller owns)
	 * @return Failure: NULL with error set (HTML2TEX_ERR_NOMEM)
	 */
	StringBuffer* string_buffer_create(size_t initial_capacity);

	/**
	 * @brief Releases all buffer resources and invalidates pointer.
	 * @param buf Buffer to destroy (NULL-safe)
	 */
	void string_buffer_destroy(StringBuffer* buf);

	/**
	 * @brief Resets buffer to empty state while preserving capacity.
	 * @param buf Buffer to clear (non-NULL)
	 * @return Success: 0
	 * @return Failure: -1 with error set
	 */
	int string_buffer_clear(StringBuffer* buf);

	/**
	 * @brief Transfers buffer ownership to caller, resetting original buffer.
	 * @param buf Buffer to detach from (non-NULL)
	 * @return Success: Null-terminated string (caller must free())
	 * @return Failure: NULL with error set
	 */
	char* string_buffer_detach(StringBuffer* buf);

	/**
	 * @brief Appends raw bytes to buffer with optional length specification.
	 * @param buf Target string buffer
	 * @param str Source string (NULL-safe, empty allowed)
	 * @param len Bytes to append (0 = use strlen(str))
	 * @return Success: 0
	 * @return Failure: -1 with error set
	 */
	int string_buffer_append(StringBuffer* buf, const char* str, size_t len);

	/**
	 * @brief Appends single character to buffer.
	 * @param buf Target string buffer
	 * @param c Character to append
	 * @return Success: 0
	 * @return Failure: -1 with error set
	 */
	int string_buffer_append_char(StringBuffer* buf, char c);

	/**
	 * @brief Formatted append with printf-style syntax.
	 * @param buf Target string buffer
	 * @param format printf format string
	 * @param ... Format arguments
	 * @return Success: Number of characters appended (≥0)
	 * @return Failure: -1 with error set
	 */
	int string_buffer_append_printf(StringBuffer* buf, const char* format, ...);

	/**
	 * @brief Appends string with LaTeX special character escaping.
	 * @param buf Target string buffer
	 * @param str Raw string to escape and append
	 * @return Success: 0
	 * @return Failure: -1 with error set
	 */
	int string_buffer_append_latex(StringBuffer* buf, const char* str);

	/**
	 * @brief Returns read-only pointer to buffer contents.
	 * @param buf String buffer to query
	 * @return Null-terminated C string
	 * @return Empty string ("") for NULL or empty buffer
	 * @return Never NULL
	 */
	const char* string_buffer_cstr(const StringBuffer* buf);

	/**
	 * @brief Returns current string length in bytes.
	 * @param buf String buffer to query
	 * @return Length in bytes (excluding null terminator)
	 * @return 0 for NULL or empty buffer
	 */
	size_t string_buffer_length(const StringBuffer* buf);

	/**
	 * @brief Checks if buffer is in error state.
	 * @param buf String buffer to check
	 * @return Non-zero: Error state active
	 * @return Zero: No error
	 */
	int string_buffer_has_error(const StringBuffer* buf);

	/**
	 * @brief Returns available capacity without reallocation.
	 * @param buf String buffer to check
	 * @return Bytes available before resize needed
	 * @return 0 for NULL or error state buffer
	 */
	size_t string_buffer_remaining(const StringBuffer* buf);

	/**
	 * @brief Guarantees buffer can hold at least needed bytes.
	 * @param buf String buffer to resize
	 * @param needed Minimum required capacity (including null terminator)
	 * @return Success: 0
	 * @return Failure: -1 with error set
	 */
	int string_buffer_ensure_capacity(StringBuffer* buf, size_t needed);

	/**
	 * @brief Explicit capacity allocation without content change.
	 * @param buf String buffer to resize
	 * @param capacity Required capacity (including null terminator)
	 * @return Success: 0
	 * @return Failure: -1 with error set
	 */
	int string_buffer_reserve(StringBuffer* buf, size_t capacity);

	/**
	 * @brief Appends entire contents of one buffer to another.
	 * @param dest Destination string buffer (modified)
	 * @param src Source string buffer (unchanged)
	 * @return Success: 0
	 * @return Failure: -1 with error set
	 */
	int string_buffer_append_buffer(StringBuffer* dest, const StringBuffer* src);

	/**
	 * @brief Safe character retrieval with bounds checking.
	 * @param buf String buffer to read from
	 * @param index Character position (0-based)
	 * @return Character at position
	 * @return '\0' for out-of-bounds or error
	 */
	char string_buffer_get_char(const StringBuffer* buf, size_t index);

	/**
	 * @brief In-place character modification.
	 * @param buf String buffer to modify
	 * @param index Position to update (0-based)
	 * @param c New character value
	 * @return Success: 0
	 * @return Failure: -1 with error set
	 */
	int string_buffer_set_char(StringBuffer* buf, size_t index, char c);

	/**
	 * @brief Reduces buffer capacity to match current content.
	 * @param buf String buffer to shrink
	 * @return Success: 0
	 * @return Failure: -1 with error set
	 */
	int string_buffer_shrink_to_fit(StringBuffer* buf);

#ifdef __cplusplus
}
#endif

#endif