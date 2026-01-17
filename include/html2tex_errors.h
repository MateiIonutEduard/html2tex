#ifndef HTML2TEX_ERRORS_H
#define HTML2TEX_ERRORS_H

#include <errno.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    /* @brief Comprehensive error codes for HTML2TeX operations. */
    typedef enum {
        HTML2TEX_OK = 0,
        HTML2TEX_ERR_NOMEM,
        HTML2TEX_ERR_BUF_OVERFLOW,
        HTML2TEX_ERR_INVAL,
        HTML2TEX_ERR_NULL,
        HTML2TEX_ERR_IO,
        HTML2TEX_ERR_FILE_OPEN,
        HTML2TEX_ERR_FILE_READ,
        HTML2TEX_ERR_FILE_WRITE,
        HTML2TEX_ERR_PARSE,
        HTML2TEX_ERR_HTML_SYNTAX,
        HTML2TEX_ERR_CSS_SYNTAX,
        HTML2TEX_ERR_MALFORMED,
        HTML2TEX_ERR_CONVERT,
        HTML2TEX_ERR_UNSUPPORTED,
        HTML2TEX_ERR_CSS,
        HTML2TEX_ERR_CSS_VALUE,
        HTML2TEX_ERR_TABLE,
        HTML2TEX_ERR_TABLE_STRUCTURE,
        HTML2TEX_ERR_IMAGE,
        HTML2TEX_ERR_IMAGE_DOWNLOAD,
        HTML2TEX_ERR_IMAGE_DECODE,
        HTML2TEX_ERR_INTERNAL,
        HTML2TEX_ERR_ASSERT,
        HTML2TEX_ERR_NETWORK,
        HTML2TEX_ERR_DOWNLOAD
    } HTML2TeXError;

#define HTML2TEX_ERROR_MSG_MAX 384

    /* @brief Thread-local error opaque context. */
    typedef struct HTML2TeXErrorCtx HTML2TeXErrorCtx;

    /**
     * @brief Retrieves the current thread's error code.
     * @return Current HTML2TeXError enum value
     * @return HTML2TEX_OK (0) if no error present
     */
    HTML2TeXError html2tex_err_get(void);

    /**
     * @brief Returns formatted error message with context.
     * @return Human-readable error description
     * @return Static buffer (do not free)
     * @return Empty string if no error
     */
    const char* html2tex_err_msg(void);

    /**
     * @brief Maps error code to static description string.
     * @param err Error code to describe
     * @return Static error description string
     * @return "Unknown error" for invalid codes
     * @return Never NULL
     */
    const char* html2tex_err_str(HTML2TeXError err);

    /**
     * @brief Resets thread's error state to HTML2TEX_OK.
     */
    void html2tex_err_clear(void);

    /**
     * @brief Captures current error state for later restoration.
     * @return Success: Opaque state handle (caller must free)
     * @return Failure: NULL (only on memory allocation failure)
     */
    void* html2tex_err_save(void);

    /**
     * @brief Restores previously saved error state.
     * @param saved Handle from html2tex_err_save() (NULL-safe)
     */
    void html2tex_err_restore(void* saved);

    /**
     * @brief Retrieves captured system errno from when error occurred.
     * @return System errno value at error time
     * @return 0 if no error or errno not captured
     * @return Preserved across library calls
     */
    int html2tex_err_syserr(void);

    /**
     * @brief Returns source filename where error originated.
     * @return Source file path (static string)
     * @return NULL if no location captured
     * @return Only set when HTML2TEX__SET_ERR macro used
     */
    const char* html2tex_err_file(void);

    /**
     * @brief Returns source line number where error occurred.
     * @return Line number (1-based)
     * @return 0 if no location captured
     * @return Only set when HTML2TEX__SET_ERR macro used
     */
    int html2tex_err_line(void);

    /**
     * @brief Quick check for error presence.
     * @return Non-zero: Error exists
     * @return Zero: No error (HTML2TEX_OK)
     */
    int html2tex_has_error(void);

    void html2tex__err_set(HTML2TeXError err, const char* fmt, ...);
    void html2tex__err_set_loc(HTML2TeXError err, const char* file, int line,
        const char* func, const char* fmt, ...);

#define HTML2TEX__SET_ERR(err, ...) \
    html2tex__err_set_loc((err), __FILE__, __LINE__, __func__, __VA_ARGS__)

#define HTML2TEX__CHECK(cond, err, ...) \
    do { \
        if (!(cond)) { \
            HTML2TEX__SET_ERR((err), __VA_ARGS__); \
            return; \
        } \
    } while(0)

#define HTML2TEX__CHECK_NULL(ptr, err, ...) \
    do { \
        if ((ptr) == NULL) { \
            HTML2TEX__SET_ERR((err), __VA_ARGS__); \
            return NULL; \
        } \
    } while(0)

#define HTML2TEX__CHECK_RET(cond, err, ret, ...) \
    do { \
        if (!(cond)) { \
            HTML2TEX__SET_ERR((err), __VA_ARGS__); \
            return (ret); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif