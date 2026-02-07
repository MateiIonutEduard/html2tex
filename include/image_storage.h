#ifndef IMAGE_STORAGE_H
#define IMAGE_STORAGE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct ImageStorage ImageStorage;

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

	/**
	 * @brief Controls deferred image downloading mode with state preservation.
	 * @param storage Double pointer to ImageStorage handle.
	 * @param enable Boolean control flag:
	 *
	 *                  - Non-zero: Enable deferred downloads
	 *
	 *                  - Zero: Disable deferred downloads
	 * @return int Success indicator:
	 *
	 *         - 1: Operation completed successfully
	 *
	 *         - 0: Operation failed, error state set (check html2tex_has_error())
	 *
	 * @note When disabling with pending downloads, existing storage is destroyed
	 *       and replaced with fresh disabled storage to ensure clean state.
	 *
	 * @see create_image_storage()
	 * @see destroy_image_storage()
	 * @see clear_image_storage()
	 */
	int html2tex_enable_downloads(ImageStorage** storage, int enable);

	/**
	 * @brief Queues image file for deferred processing in lazy download mode.
	 *
	 * @param storage Double pointer to ImageStorage instance for deferred processing.
	 *
	 *                       Storage must be initialized via html2tex_enable_downloads()
	 *
	 *                       with lazy_downloading enabled.
	 *
	 * @param file_path Absolute or relative path to image file for deferred processing.
	 *
	 *                     Must be a valid, non-empty string representing an existing file.
	 * @return Status code indicating operation result:
	 *
	 *         -  Success: 1, image queued for deferred processing
	 *
	 *         -  Success: 0, lazy downloading not enabled or storage is NULL (no operation)
	 *
	 *         - Failure: -1, error state set (check html2tex_has_error())
	 *
	 * @note The function validates file_path existence and accessibility during
	 *       actual processing (clear_image_storage()), not during queuing.
	 *
	 * @warning Path length validation uses SIZE_MAX define. Ensure this
	 *          matches system constraints.
	 *
	 * @see html2tex_enable_downloads()
	 * @see clear_image_storage()
	 * @see destroy_image_storage()
	 */
	int html2tex_add_image(ImageStorage** storage, const char* file_path);

	/**
	 * @brief Creates a deep copy of an ImageStorage structure with all contained image references.
	 * @param store Source ImageStorage to copy (non-NULL)
	 * 
	 * @note The copy operation is atomic with respect to error state - either complete
	 *       success or complete failure with all resources cleaned up.
	 * @warning Not thread-safe if source is modified during copy operation
	 * @see create_image_storage() for constructor
	 * @see destroy_image_storage() for destructor
	 * @see clear_image_storage() for partial cleanup
	 * 
	 * @return Success: Independent ImageStorage* copy (caller must destroy)
	 * @return Failure: NULL with error state set (check html2tex_has_error())
	 */
	ImageStorage* copy_image_storage(const ImageStorage* store);

#ifdef __cplusplus
}
#endif

#endif