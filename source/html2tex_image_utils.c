#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef _WIN32
#define _WINSOCKAPI_
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <winsock2.h>

#include <sys/types.h>
#include <sys/stat.h>

#define stat _stat
#define fstat _fstat
#define fileno _fileno
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include <curl/curl.h>
#include "html2tex.h"
#include <time.h>

/* Base64 decoding table */
static const unsigned char base64_table[256] = {
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x3E, 0x80, 0x80, 0x80, 0x3F,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80,
    0x80, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};

int is_base64_image(const char* src) {
    if (!src) return 0;
    return (strncmp(src, "data:image/", 11) == 0);
}

/* @brief Extract MIME type from base64 data URI. */
static char* extract_mime_type(const char* base64_data) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (!base64_data) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Base64 data URI is NULL for "
            "MIME type extraction.");
        return NULL;
    }

    const char* prefix = "data:";
    const char* semicolon = strchr(base64_data, ';');

    if (!semicolon) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Malformed data URI: missing semicolon "
            "after MIME type.");
        return NULL;
    }

    size_t len = semicolon - (base64_data + strlen(prefix));

    if (len == 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Empty MIME type in data URI.");
        return NULL;
    }

    char* mime_type = (char*)malloc(len + 1);

    if (!mime_type) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate %zu bytes for"
            " MIME type.", len + 1);
        return NULL;
    }

    strncpy(mime_type, base64_data + strlen(prefix), len);
    mime_type[len] = '\0';
    return mime_type;
}

/* @brief Extract base64 data from data URI. */
static char* extract_base64_data(const char* base64_data) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (!base64_data) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Base64 data URI is NULL for "
            "data extraction.");
        return NULL;
    }

    const char* base64_prefix = "base64,";
    const char* data_start = strstr(base64_data, base64_prefix);

    if (!data_start) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Malformed data URI: missing base64 prefix.");
        return NULL;
    }

    data_start += strlen(base64_prefix);

    /* validate we have data after the prefix */
    if (*data_start == '\0') {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Empty base64 data after prefix.");
        return NULL;
    }

    /* calculate length and allocate efficiently */
    size_t len = strlen(data_start);
    size_t clean_len = 0;

    /* pre-scan to determine exact memory needed */
    for (const char* src = data_start; *src; src++) {
        if (!isspace((unsigned char)*src)) {
            clean_len++;
        }
    }

    if (clean_len == 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Base64 data contains only whitespace.");
        return NULL;
    }

    char* clean_data = (char*)malloc(clean_len + 1);

    if (!clean_data) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate %zu bytes for base64 data.",
            clean_len + 1);
        return NULL;
    }

    /* copy and clean the data */
    char* dest = clean_data;

    for (const char* src = data_start; *src; src++) {
        if (!isspace((unsigned char)*src)) {
            *dest++ = *src;
        }
    }

    *dest = '\0';

    /* validate the cleaned data looks like base64 */
    if (clean_len % 4 != 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Invalid base64 data length: %zu (must be multiple of 4).",
            clean_len);
        free(clean_data);
        return NULL;
    }

    /* quick sanity check for base64 characters */
    for (size_t i = 0; i < clean_len; i++) {
        char c = clean_data[i];
        if (!((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '+' || c == '/' || c == '=')) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
                "Invalid base64 character '%c' (0x%02X) at position %zu.",
                (c < 32 || c > 126) ? '?' : c, (unsigned char)c, i);
            free(clean_data);
            return NULL;
        }
    }

    return clean_data;
}

/* @brief Get file extension from MIME type. */
static const char* get_extension_from_mime_type(const char* mime_type) {
    if (!mime_type) return ".bin";

    if (strstr(mime_type, "jpeg") || strstr(mime_type, "jpg")) return ".jpg";
    else if (strstr(mime_type, "png")) return ".png";
    else if (strstr(mime_type, "gif")) return ".gif";
    else if (strstr(mime_type, "bmp")) return ".bmp";
    else if (strstr(mime_type, "tiff")) return ".tiff";
    else if (strstr(mime_type, "webp")) return ".webp";
    else if (strstr(mime_type, "svg")) return ".svg";
    else return ".bin";
}

/* @brief Robust base64 decoding function. */
static unsigned char* base64_decode(const char* data, size_t input_len, size_t* output_len) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (!data) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Base64 data is NULL for decoding.");
        return NULL;
    }

    if (input_len == 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Empty base64 data for decoding.");
        return NULL;
    }

    /* invalid base64 string length */
    if (input_len % 4 != 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Invalid base64 length: %zu (must be multiple of 4).",
            input_len);
        return NULL;
    }

    /* calculate output length with padding handling */
    *output_len = (input_len * 3) / 4;

    if (data[input_len - 1] == '=') (*output_len)--;
    if (data[input_len - 2] == '=') (*output_len)--;

    if (*output_len == 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Zero-length output after base64 padding removal.");
        return NULL;
    }

    /* validate maximum reasonable output size */
    const size_t MAX_REASONABLE_SIZE = 16 * 1024 * 1024;

    if (*output_len > MAX_REASONABLE_SIZE) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Decoded base64 size %zu exceeds maximum allowed %zu bytes.",
            *output_len, MAX_REASONABLE_SIZE);
        return NULL;
    }

    unsigned char* decoded = (unsigned char*)malloc(*output_len);
    if (!decoded) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate %zu bytes for decoded base64 data.",
            *output_len);
        return NULL;
    }

    const unsigned char* input = (const unsigned char*)data;
    unsigned char* output = decoded;

    for (size_t i = 0; i < input_len; ) {
        /* decode 4 characters to 3 bytes */
        unsigned long int sextet_a = (input[i] == '=') ? 
            0 : base64_table[input[i]];
        i++;

        unsigned long int sextet_b = (input[i] == '=') ? 
            0 : base64_table[input[i]];
        i++;

        unsigned long int sextet_c = (input[i] == '=') ? 
            0 : base64_table[input[i]];
        i++;

        unsigned long int sextet_d = (input[i] == '=') ? 
            0 : base64_table[input[i]];
        i++;

        /* validate decoded values */
        if (sextet_a == 0x80 || sextet_b == 0x80 ||
            sextet_c == 0x80 || sextet_d == 0x80) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
                "Invalid base64 character at position %zu: '%c' (0x%02X).",
                i - 1, data[i - 1], (unsigned char)data[i - 1]);
            free(decoded);
            return NULL;
        }

        /* combine into 24-bit value */
        unsigned long int triple = (sextet_a << 18) | 
            (sextet_b << 12) |
            (sextet_c << 6) | 
            sextet_d;

        /* extract 3 bytes with bounds checking */
        if (output - decoded < (ptrdiff_t)(*output_len - 2))
            *output++ = (triple >> 16) & 0xFF;

        if (output - decoded < (ptrdiff_t)(*output_len - 1))
            *output++ = (triple >> 8) & 0xFF;

        if (output - decoded < (ptrdiff_t)(*output_len))
            *output++ = triple & 0xFF;
    }

    /* verify it wrote the expected amount */
    size_t bytes_written = (size_t)(output - decoded);

    if (bytes_written != *output_len) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INTERNAL,
            "Base64 decoding inconsistency: "
            "expected %zu bytes, wrote %zu.",
            *output_len, bytes_written);
        free(decoded);
        return NULL;
    }

    return decoded;
}

/* @brief Decode base64 data and save to file. */
static int save_base64_image(const char* base64_data, const char* filename) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (!base64_data) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Base64 data is NULL for image save.");
        return 0;
    }

    if (!filename) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Filename is NULL for base64 image save.");
        return 0;
    }

    char* clean_data = extract_base64_data(base64_data);
    /* error already set by extract_base64_data */
    if (!clean_data) return 0;
    size_t input_len = strlen(clean_data);

    if (input_len == 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Cleaned base64 data is empty.");
        free(clean_data);
        return 0;
    }

    size_t output_len = 0;
    unsigned char* decoded_data = base64_decode(
        clean_data, input_len, &output_len);
    free(clean_data);

    /* error already set by base64_decode */
    if (!decoded_data) return 0;

    if (output_len == 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DECODE,
            "Base64 decoding produced zero-length output.");
        free(decoded_data);
        return 0;
    }

    /* write to the file */
    FILE* file = fopen(filename, "wb");

    if (!file) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_FILE_OPEN,
            "Failed to open file '%s' for writing: %s.",
            filename, strerror(errno));
        free(decoded_data);
        return 0;
    }

    size_t written = fwrite(decoded_data, 
        1, output_len, file);

    /* close file before checking write result */
    if (fclose(file) != 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_FILE_WRITE,
            "Failed to close file '%s' after write: %s.",
            filename, strerror(errno));
        free(decoded_data);
        return 0;
    }

    free(decoded_data);

    if (written != output_len) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_FILE_WRITE,
            "Failed to write complete image data to '%s': "
            "expected %zu bytes, wrote %zu bytes.",
            filename, output_len, written);
        return 0;
    }

    return 1;
}

/* @brief libcurl write callback */
static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

/* @brief Download image from URL using libcurl. */
static int download_image_url(const char* url, const char* filename) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (!url) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "URL is NULL for image download.");
        return 0;
    }

    if (!filename) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Filename is NULL for image download.");
        return 0;
    }

    CURL* curl = NULL;
    FILE* fp = NULL;
    CURLcode res;
    int success = 0;

    curl = curl_easy_init();
    if (!curl) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DOWNLOAD,
            "Failed to initialize libcurl handle.");
        return 0;
    }

    fp = fopen(filename, "wb");
    if (!fp) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_FILE_OPEN,
            "Failed to open file '%s' for writing: %s.",
            filename, strerror(errno));
        curl_easy_cleanup(curl);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "html2tex/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long response_code = 0;
        curl_easy_getinfo(curl, 
            CURLINFO_RESPONSE_CODE, 
            &response_code);

        if (response_code == 200)
            success = 1;
        else {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DOWNLOAD,
                "HTTP request failed with status code: %ld for URL: %s.",
                response_code, url);
        }
    }
    else {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_IMAGE_DOWNLOAD,
            "libcurl error: %s for URL: %s",
            curl_easy_strerror(res), url);
    }

    if (fp) {
        if (fclose(fp) != 0) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_FILE_WRITE,
                "Failed to close file '%s' after download: %s.",
                filename, strerror(errno));
        }
    }

    if (curl) curl_easy_cleanup(curl);
    return success;
}

/* @brief Create directory if it doesn't exist. */
static int create_directory_if_not_exists(const char* dir_path) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (!dir_path) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Directory path is NULL for creation.");
        return -1;
    }

    if (dir_path[0] == '\0') {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INVAL,
            "Directory path is empty string.");
        return -1;
    }

    struct stat st = { 0 };

    if (stat(dir_path, &st) == -1) {
        /* create directory recursively */
        char* path_copy = strdup(dir_path);

        if (!path_copy) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to duplicate directory path: %s.", dir_path);
            return -1;
        }

        char* p = path_copy;

#ifdef _WIN32
        /* skip Windows drive prefix for recursive creation */
        if (strlen(p) > 2 && p[1] == ':')
            p += 2;
#endif

        while (*p) {
            if (*p == '/' || *p == '\\') {
                char old_char = *p;
                *p = '\0';

                if (stat(path_copy, &st) == -1) {
                    if (mkdir(path_copy) != 0) {
                        HTML2TEX__SET_ERR(HTML2TEX_ERR_IO,
                            "Failed to create directory '%s': %s.",
                            path_copy, strerror(errno));
                        free(path_copy);
                        return -1;
                    }
                }

                *p = old_char;
            }

            p++;
        }

        /* create the final directory */
        if (mkdir(dir_path) != 0) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_IO,
                "Failed to create final directory '%s': %s.",
                dir_path, strerror(errno));
            free(path_copy);
            return -1;
        }

        free(path_copy);
    }

    return 0;
}

/* @brief Generate safe filename from URL or base64 data. */
static char* generate_safe_filename(const char* src, int image_counter) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (!src) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Source is NULL for filename generation.");
        return NULL;
    }

    if (image_counter < 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INVAL,
            "Invalid image counter: %d (must be non-negative).",
            image_counter);
        return NULL;
    }

    char* filename = (char*)malloc(256);

    if (!filename) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate 256 bytes for filename buffer.");
        return NULL;
    }

    if (is_base64_image(src)) {
        char* mime_type = extract_mime_type(src);

        /* error already set by extract_mime_type */
        if (!mime_type) {
            free(filename);
            return NULL;
        }

        const char* extension = get_extension_from_mime_type(mime_type);

        if (!extension) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_INTERNAL,
                "Failed to get extension for MIME type: %s.", mime_type);
            free(mime_type);
            free(filename);
            return NULL;
        }

        int result = snprintf(filename, 256, "image_%d%s",
            image_counter, extension);
        free(mime_type);

        if (result < 0 || result >= 256) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                "Generated filename exceeds buffer size for counter: %d.",
                image_counter);
            free(filename);
            return NULL;
        }
    }
    else {
        /* extract filename from URL */
        const char* last_slash = strrchr(src, '/');
        const char* name_start = last_slash ? last_slash + 1 : src;

        /* empty filename in URL */
        if (!*name_start) {
            int result = snprintf(filename, 256, "image_%d.jpg", image_counter);
            if (result < 0 || result >= 256) {
                HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                    "Default filename exceeds buffer for counter: %d.",
                    image_counter);
                free(filename);
                return NULL;
            }

            return filename;
        }

        /* find end of filename, before query or fragment */
        const char* end = name_start;
        while (*end && *end != '?' && *end != '#' && *end != ';')
            end++;

        size_t name_len = end - name_start;

        if (name_len == 0) {
            int result = snprintf(filename, 256, 
                "image_%d.jpg", image_counter);

            if (result < 0 || result >= 256) {
                HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                    "Default filename exceeds buffer for counter: %d.",
                    image_counter);
                free(filename);
                return NULL;
            }
            return filename;
        }

        /* copy and sanitize the filename */
        char* dest = filename;
        const char* src_char = name_start;
        size_t copied = 0;

        while (src_char < end && copied < 255) {
            char c = *src_char++;

            if (isalnum((unsigned char)c) || 
                c == '.' || c == '-' || 
                c == '_') {
                *dest++ = c;
                copied++;
            }
            else {
                *dest++ = '_';
                copied++;
            }
        }

        *dest = '\0';

        /* ensure it has an extension */
        char* dot = strrchr(filename, '.');

        if (!dot || dot == filename || (dot - filename) < 2) {
            size_t current_len = strlen(filename);
            if (current_len > 251) {
                HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                    "Filename too long to add .jpg extension: %s.",
                    filename);
                free(filename);
                return NULL;
            }

            strncat(filename, ".jpg", 
                255 - current_len);
        }
    }

    return filename;
}

/* Generate deterministic hash (djb2) from input. */
static unsigned long deterministic_hash(const char* str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    
    return hash;
}

/* @brief Generate unique filename for the specified image. */
static char* generate_unique_filename(const char* output_dir, const char* src, int image_counter) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (!output_dir) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Output directory is NULL for "
            "unique filename generation.");
        return NULL;
    }

    if (!src) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Source is NULL for unique filename"
            " generation.");
        return NULL;
    }

    if (image_counter < 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INVAL,
            "Invalid image counter: %d (must "
            "be non-negative).",
            image_counter);
        return NULL;
    }

    char* filename = generate_safe_filename(src, image_counter);
    /* error already set by generate_safe_filename */
    if (!filename) return NULL;

    /* check if file already exists */
    char full_path[1024];
    int snprintf_result = snprintf(full_path, sizeof(full_path), 
        "%s/%s", output_dir, filename);

    if (snprintf_result < 0 || snprintf_result >= (int)sizeof(full_path)) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Full path exceeds buffer size for: %s/%s.",
            output_dir, filename);
        free(filename);
        return NULL;
    }

    struct stat st;

    /* file does not exist, use this filename */
    if (stat(full_path, &st) != 0)
        return filename;

    /* file exists, apply deterministic hash */
    free(filename);
    unsigned long hash = deterministic_hash(src);

    char hash_str[9];
    snprintf_result = snprintf(hash_str, sizeof(hash_str),
        "%08lx", hash);

    if (snprintf_result < 0 || snprintf_result >= (int)sizeof(hash_str)) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INTERNAL,
            "Hash string formatting failed for hash: %lx.", hash);
        return NULL;
    }

    if (is_base64_image(src)) {
        char* mime_type = extract_mime_type(src);
        if (!mime_type) {
            /* error already set by extract_mime_type */
            return NULL;
        }

        const char* extension = get_extension_from_mime_type(mime_type);
        if (!extension) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_INTERNAL,
                "Failed to get extension for MIME type: %s.", mime_type);
            free(mime_type);
            return NULL;
        }

        char* unique_name = (char*)malloc(256);
        if (!unique_name) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to allocate 256 bytes for unique filename.");
            free(mime_type);
            return NULL;
        }

        snprintf_result = snprintf(unique_name, 256, "image_%d_%s%s",
            image_counter, hash_str, extension);
        free(mime_type);

        if (snprintf_result < 0 || snprintf_result >= 256) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                "Unique filename exceeds buffer for counter: %d, hash: %s.",
                image_counter, hash_str);
            free(unique_name);
            return NULL;
        }

        return unique_name;
    }

    /* for URLs, extract base name and add hash before extension */
    const char* last_slash = strrchr(src, '/');
    const char* name_start = last_slash ? last_slash + 1 : src;

    /* empty filename */
    if (!*name_start) {
        char* unique_name = (char*)malloc(256);
        if (!unique_name) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
                "Failed to allocate 256 bytes for"
                " default unique filename.");
            return NULL;
        }

        snprintf_result = snprintf(unique_name, 256, "image_%d_%s.jpg",
            image_counter, hash_str);
        if (snprintf_result < 0 || snprintf_result >= 256) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                "Default unique filename exceeds buffer"
                " for counter: %d.", image_counter);
            free(unique_name);
            return NULL;
        }

        return unique_name;
    }

    /* find filename boundaries */
    const char* end = name_start;
    while (*end && *end != '?' 
        && *end != '#' 
        && *end != ';')
        end++;

    /* find last dot in the filename part */
    const char* last_dot = NULL;

    for (const char* p = name_start; p < end; p++) {
        if (*p == '.') 
            last_dot = p;
    }

    char* unique_name = (char*)malloc(256);

    if (!unique_name) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate 256 bytes for"
            " URL unique filename.");
        return NULL;
    }

    /* has extension */
    if (last_dot && last_dot > name_start) {
        size_t name_len = last_dot - name_start;
        size_t ext_len = end - last_dot;

        if (name_len > 100) name_len = 100;
        if (ext_len > 10) ext_len = 10;

        /* copy name part */
        char* dest = unique_name;
        const char* src_char = name_start;
        size_t copied = 0;

        while (src_char < last_dot && copied < 100) {
            char c = *src_char++;
            if (isalnum((unsigned char)c) 
                || c == '-' || c == '_') {
                *dest++ = c;
                copied++;
            }
            else {
                *dest++ = '_';
                copied++;
            }
        }

        /* add hash and extension */
        size_t remaining = 256 - (dest - unique_name);
        snprintf_result = snprintf(dest, remaining, "_%s%.*s",
            hash_str, (int)ext_len, last_dot);

        if (snprintf_result < 0 || (size_t)snprintf_result >= remaining) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                "Filename with extension exceeds buffer: %s.", src);
            free(unique_name);
            return NULL;
        }
    }
    else {
        /* no extension found */
        size_t name_len = end - name_start;
        if (name_len > 100) name_len = 100;

        /* copy sanitized name */
        char* dest = unique_name;
        const char* src_char = name_start;
        size_t copied = 0;

        if (src_char) {
            while (src_char < end && copied < 100) {
                char c = *src_char++;

                if (isalnum((unsigned char)c) || c == '-' || c == '_') {
                    *dest++ = c;
                    copied++;
                }
                else {
                    *dest++ = '_';
                    copied++;
                }
            }
        }

        /* add hash and .jpg extension */
        size_t remaining = 256 - (dest - unique_name);

        snprintf_result = snprintf(dest, remaining, 
            "_%s.jpg", hash_str);

        if (snprintf_result < 0 || (size_t)snprintf_result >= remaining) {
            HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
                "Filename without extension exceeds buffer: %s.", src);
            free(unique_name);
            return NULL;
        }
    }

    return unique_name;
}

ImageStorage* create_image_storage() {
    /* clear any previous error state */
    html2tex_err_clear();

    ImageStorage* store = (ImageStorage*)malloc(sizeof(ImageStorage));

    if (!store) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate %zu bytes "
            "for ImageStorage structure.",
            sizeof(ImageStorage));
        return NULL;
    }

    /* initialize with safe defaults */
    store->lazy_downloading = 0;
    store->image_stack = NULL;
    return store;
}

int clear_image_storage(ImageStorage* store, int enable) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (store && enable) {
        Stack* image_stack = store->image_stack;
        size_t count = 0;

        char** filenames = (char**)stack_to_array(&image_stack, &count);
        int errorThrown = html2tex_has_error();

        /* propagate the existing error forwardly */
        if (errorThrown) return -1;

        /* transferring data ownership succeed */
        for (size_t i = 0; i < count; i++)
            free(filenames[i]);

        free(filenames);
        return 1;
    }

    return 0;
}

char* download_image_src(const char* src, const char* output_dir, int image_counter) {
    /* clear any existing error state */
    html2tex_err_clear();

    if (!src) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Image source is NULL for download.");
        return NULL;
    }

    if (!output_dir) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NULL,
            "Output directory is NULL for image download.");
        return NULL;
    }

    if (image_counter < 0) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_INVAL,
            "Invalid image counter: %d (must be non-negative).",
            image_counter);
        return NULL;
    }

    /* create output directory if it does not exist */
    if (create_directory_if_not_exists(output_dir) != 0)
        return NULL;

    /* generate unique filename */
    char* safe_filename = generate_unique_filename(output_dir, src, image_counter);
    if (!safe_filename) return NULL;

    /* build full path with exact length calculation */
    size_t dir_len = strlen(output_dir);
    size_t filename_len = strlen(safe_filename);

    /* check for overflow in path length calculation */
    if (dir_len > SIZE_MAX - filename_len - 2) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Path length overflow: dir=%zu, filename=%zu.",
            dir_len, filename_len);
        free(safe_filename);
        return NULL;
    }

    size_t full_path_len = dir_len + filename_len + 2;
    char* full_path = (char*)malloc(full_path_len);

    if (!full_path) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_NOMEM,
            "Failed to allocate %zu bytes for full path.",
            full_path_len);
        free(safe_filename);
        return NULL;
    }

    int snprintf_result = snprintf(full_path, full_path_len, "%s/%s",
        output_dir, safe_filename);

    if (snprintf_result < 0 || (size_t)snprintf_result >= full_path_len) {
        HTML2TEX__SET_ERR(HTML2TEX_ERR_BUF_OVERFLOW,
            "Generated full path exceeds allocated buffer: %s/%s.",
            output_dir, safe_filename);
        free(safe_filename);
        free(full_path);
        return NULL;
    }

    int success = 0;

    /* handle base64 encoded image */
    if (is_base64_image(src))
        success = save_base64_image(src, full_path);
    else
        /* handle normal URL */
        success = download_image_url(src, full_path);
    free(safe_filename);

    if (success)
        return full_path;
    else {
        free(full_path);
        return NULL;
    }
}

int image_utils_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    return 0;
}

void image_utils_cleanup(void) {
    curl_global_cleanup();
}