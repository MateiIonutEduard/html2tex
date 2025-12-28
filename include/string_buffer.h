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

	/* Create a new StringBuffer with specified initial capacity. */
	StringBuffer* string_buffer_create(size_t initial_capacity);

	/* Create a new StringBuffer with specified initial capacity. */
	void string_buffer_destroy(StringBuffer* buf);

	/* Clear buffer contents while preserving capacity. */
	int string_buffer_clear(StringBuffer* buf);

	/* Detach buffer memory, transferring ownership to caller. */
	char* string_buffer_detach(StringBuffer* buf);

	/* Append string to the output buffer. */
	int string_buffer_append(StringBuffer* buf, const char* str, size_t len);

	/* Append single character to the output buffer. */
	int string_buffer_append_char(StringBuffer* buf, char c);

	/* Append formatted string in printf-style. */
	int string_buffer_append_printf(StringBuffer* buf, const char* format, ...);

	/* Append string with the LaTeX escaping. */
	int string_buffer_append_latex(StringBuffer* buf, const char* str);

	/* Get the current string content. */
	const char* string_buffer_cstr(const StringBuffer* buf);

	/* Get the current string length. */
	size_t string_buffer_length(const StringBuffer* buf);

	/* Check if buffer is in error state. */
	int string_buffer_has_error(const StringBuffer* buf);

	/* Get remaining capacity, excluding null terminator. */
	size_t string_buffer_remaining(const StringBuffer* buf);

	/* Ensure the buffer has at least specified capacity. */
	int string_buffer_ensure_capacity(StringBuffer* buf, size_t needed);

	/* Shrink buffer to fit the current content. */
	int string_buffer_shrink_to_fit(StringBuffer* buf);

#ifdef __cplusplus
}
#endif

#endif