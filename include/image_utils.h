#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	/* Downloads an image from the specified URL. */
	char* download_image_src(const char* src, const char* output_dir, int image_counter);

	/* Returns whether src contains a base64-encoded image. */
	int is_base64_image(const char* src);

	/* Initializes download processing. */
	int image_utils_init(void);

	/* Frees image-download resources. */
	void image_utils_cleanup(void);
#ifdef __cplusplus
}
#endif

#endif