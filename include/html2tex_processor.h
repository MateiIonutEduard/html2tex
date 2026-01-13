#ifndef HTML2TEX_PROCESSOR_H
#define HTML2TEX_PROCESSOR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct LaTeXConverter LaTeXConverter;
	typedef struct HTMLNode HTMLNode;

	/** 
	 * @brief Determines if HTML element is supported for LaTeX conversion.
	 * @param node HTML DOM node to evaluate
	 * @return 1: Element is explicitly supported for conversion
	 * @return 0: Element is not supported, invalid, or NULL 
	 */
	int is_supported_element(const HTMLNode* node);

	/**
	 * @brief Converts HTML element to LaTeX with proper start/end semantics.
	 * @param converter Active LaTeX conversion context
	 * @param node HTML element to convert
	 * @param is_starting Conversion phase selector
	 * @return -1: Invalid parameters (converter NULL, node invalid, or unsupported element)
	 * @return 0: Element processed but no LaTeX output generated (e.g., div, span, font)
	 * @return 1: Successful conversion operation (LaTeX output generated)
	 * @return 2: Self-closing element processed (e.g., br, hr)
	 */
	int convert_element(LaTeXConverter* converter, const HTMLNode* node, bool is_starting);

#ifdef __cplusplus
}
#endif

#endif