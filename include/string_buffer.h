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

	/* @brief Create a new StringBuffer with specified initial capacity. */
	StringBuffer* string_buffer_create(size_t initial_capacity);

	/* @brief Create a new StringBuffer with specified initial capacity. */
	void string_buffer_destroy(StringBuffer* buf);

	/* @brief Clear buffer contents while preserving capacity. */
	int string_buffer_clear(StringBuffer* buf);

	/* @brief Detach buffer memory, transferring ownership to caller. */
	char* string_buffer_detach(StringBuffer* buf);

	/* @brief Append string to the output buffer. */
	int string_buffer_append(StringBuffer* buf, const char* str, size_t len);

	/* @brief Append single character to the output buffer. */
	int string_buffer_append_char(StringBuffer* buf, char c);

	/* @brief Append formatted string in printf-style. */
	int string_buffer_append_printf(StringBuffer* buf, const char* format, ...);

	/* @brief Append string with the LaTeX escaping. */
	int string_buffer_append_latex(StringBuffer* buf, const char* str);

	/* @brief Get the current string content. */
	const char* string_buffer_cstr(const StringBuffer* buf);

	/* @brief Get the current string length. */
	size_t string_buffer_length(const StringBuffer* buf);

	/* @brief Check if buffer is in error state. */
	int string_buffer_has_error(const StringBuffer* buf);

	/* @brief Get remaining capacity, excluding null terminator. */
	size_t string_buffer_remaining(const StringBuffer* buf);

	/* @brief Ensure the buffer has at least specified capacity. */
	int string_buffer_ensure_capacity(StringBuffer* buf, size_t needed);

	/* @brief Explicitly allocate capacity for future appends without changing content. */
	int string_buffer_reserve(StringBuffer* buf, size_t capacity);

	/* @brief Append entire contents of one buffer to another. */
	int string_buffer_append_buffer(StringBuffer* dest, const StringBuffer* src);

	/* @brief Safe character retrieval by index with bounds checking. */
	char string_buffer_get_char(const StringBuffer* buf, size_t index);

	/* @brief In-place character modification with bounds validation. */
	int string_buffer_set_char(StringBuffer* buf, size_t index, char c);

	/* @brief Shrink buffer to fit the current content. */
	int string_buffer_shrink_to_fit(StringBuffer* buf);

#ifdef __cplusplus
}
#endif

#endif