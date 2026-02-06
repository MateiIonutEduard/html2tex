#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <stddef.h>
#include "html2tex_stack.h"

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct ImageStorage ImageStorage;

	/**
	 * @brief Downloads or processes image source to local file with collision avoidance.
	 * @param src Image source (URL, file path, or Base64 data URI)
	 * @param output_dir Directory for downloaded images (created if needed)
	 * @param image_counter Sequence number for unique filename generation
	 * @return Success: Local file path (caller must free())
	 * @return Failure: NULL with error set
	 */
	char* download_image_src(const char* src, const char* output_dir, int image_counter);

	/**
	 * @brief Detects Base64-encoded image data URIs.
	 * @param src String to test
	 * @return 1: Valid Base64 image data URI
	 * @return 0: Not a Base64 image
	 */
	int is_base64_image(const char* src);

	/**
	 * @brief Initializes image download subsystem.
	 * @return Success: 0
	 * @return Failure: -1 with error set
	 */
	int image_utils_init(void);

	/* @brief Releases image download resources. */
	void image_utils_cleanup(void);

	/**
	 * @brief Allocates and initializes a new ImageStorage structure for deferred image management.
	 *
	 * @see destroy_image_storage() for complementary destruction
	 * @see html2tex_create() for converter-level integration
	 * @see html2tex_err_clear() for error state management
	 * @see ImageStorage for structure definition and field semantics
	 *
	 * @return Pointer to initialized ImageStorage structure on success.
	 * @return NULL on failure with error state set (check html2tex_has_error()).
	 */
	ImageStorage* create_image_storage();

	/**
	 * @brief Safely clears accumulated image storage with comprehensive error handling.
	 *
	 * @param store Pointer to ImageStorage structure (may be NULL).
	 *
	 * @return 1 on successful clearing with all resources freed.
	 * @return 0 when store is NULL (no operation performed).
	 * @return -1 on allocation failure with stack cleaned up and error preserved.
	 * 
	 * @warning This function transfers ownership of filename pointers from the
	 *          stack to the temporary array. Caller must ensure no other
	 *          references exist to these strings after calling.
	 *
	 * @note On failure (-1), the stack is destroyed via stack_destroy(), ensuring
	 *       no memory leaks from partial allocations.
	 *
	 * @see stack_to_array() for ownership transfer mechanism
	 * @see stack_destroy() for failure cleanup
	 * @see destroy_image_storage() for complete destruction
	 */
	int clear_image_storage(ImageStorage* store);

	/**
	 * @brief Completely destroys an ImageStorage structure with guaranteed cleanup.
	 * @param store Pointer to ImageStorage structure to destroy (may be NULL).
	 *
	 * @note This function implements **destructor semantics** rather than
	 *       **resource management semantics**. For applications requiring
	 *       guaranteed cleanup, use the explicit pattern shown above.
	 *
	 * @note The function is now more reliable due to clear_image_storage()'s
	 *       enhanced error recovery. Stack memory is cleaned up even on failures.
	 *
	 * @see clear_image_storage() for the improved cleanup implementation
	 * @see create_image_storage() for complementary constructor
	*/
	void destroy_image_storage(ImageStorage* store);

#ifdef __cplusplus
}
#endif

#endif