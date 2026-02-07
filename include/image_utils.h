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

#ifdef __cplusplus
}
#endif

#endif