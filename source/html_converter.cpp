#include "htmltex_converter.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>

HtmlTeXConverter::HtmlTeXConverter() : converter(nullptr, &html2tex_destroy), valid(false),
downloads_enabled(false), image_directory(""), image_manager(nullptr) {
    LaTeXConverter* raw_converter = html2tex_create();

    if (raw_converter) {
        converter.reset(raw_converter);
        valid = true;
    }
    else {
        /* propagates existing error */
        if (html2tex_has_error())
            throw LaTeXRuntimeException::fromLaTeXError();
    }
}

HtmlTeXConverter::HtmlTeXConverter(const HtmlTeXConverter& other)
: converter(nullptr, &html2tex_destroy), valid(false),
downloads_enabled(false), image_directory(""), image_manager(nullptr) {
    if (other.converter && other.valid) {
        LaTeXConverter* clone = html2tex_copy(other.converter.get());

        if (clone) {
            downloads_enabled = other.downloads_enabled;
            image_directory = other.image_directory;

            converter.reset(clone);
            valid = true;
        }
        else {
            /* throws the existing error */
            if (html2tex_has_error())
                throw LaTeXRuntimeException::fromLaTeXError();
        }
    }
}

HtmlTeXConverter::HtmlTeXConverter(HtmlTeXConverter&& other) noexcept
    : converter(std::move(other.converter)), valid(other.valid), 
    downloads_enabled(other.downloads_enabled), 
    image_directory(other.image_directory),
    image_manager(std::move(other.image_manager)) {
    other.downloads_enabled = false;
    other.image_directory = "";

    other.converter.reset(nullptr);
    other.image_manager.reset(nullptr);
    other.valid = false;
}

bool HtmlTeXConverter::setDirectory(const std::string& fullPath) noexcept {
    if (!converter || !valid) 
        return false;

    /* store the directory */
    image_directory = fullPath;
    downloads_enabled = true;

    html2tex_set_image_directory(converter.get(), fullPath.c_str());
    html2tex_set_download_images(converter.get(), 1);
    return true;
}

bool HtmlTeXConverter::enableLazyDownloading(bool enabled) {
    if (!converter || !valid) {
        THROW_RUNTIME_ERROR(
            "HtmlTeXConverter: "
            "Converter not initialized.", 
            -1);
    }

    bool succeed = html2tex_enable_downloads(
        &converter.get()->store,
        enabled
    );

    if (!succeed) {
        if (html2tex_has_error())
            throw LaTeXRuntimeException::fromLaTeXError();
    }

    return true;
}

ImageManager& HtmlTeXConverter::getImageManager() {
    if (!converter || !valid)
        THROW_RUNTIME_ERROR(
            "HtmlTeXConverter:"
            " Converter not "
            "initialized.", -1);

    if (!image_manager) {
        image_manager = std::make_unique<ImageManager>();

        /* check if image directory is set */
        if (image_directory.empty()) {
            THROW_RUNTIME_ERROR(
                "HtmlTeXConverter: Image directory must be set "
                "before accessing ImageManager. Call setDirectory() first.",
                -2);
        }
    }

    return *image_manager;
}

std::vector<ImageManager::DownloadRequest> HtmlTeXConverter::getImages() {
    if (!converter || !valid)
        THROW_RUNTIME_ERROR(
            "HtmlTeXConverter:"
            " Converter not "
            "initialized.", -1);

    std::vector<ImageManager::DownloadRequest> batch;
    ImageStorage* storage = converter.get()->store;

    bool enabled = (storage != nullptr) 
        && storage->lazy_downloading;
    size_t count = 0;

    if (enabled && !stack_is_empty(storage->image_stack)) {
        char** filenames = (char**)stack_to_array(
            &storage->image_stack, &count);

        if (html2tex_has_error())
            throw LaTeXRuntimeException::fromLaTeXError();

        if (count > 0) {
            for (size_t i = 0; i < count; i++) {
                std::string full_path = std::string(filenames[i]);
                std::free(filenames[i]);
                ImageManager::DownloadRequest req{ full_path, image_directory, i + 1 };
                batch.emplace_back(req);
            }

            std::free(filenames);
        }
    }

    return batch;
}

void HtmlTeXConverter::downloadQueuedImagesAsync() {
    auto image_list = getImages();

    if (!image_list.empty()) {
        ImageManager& manager = getImageManager();
        auto results = manager.downloadBatch(image_list);

        /* check downloading succeed */
        for (const auto& res : results) {
            if (!res.success)
                std::cerr << "Failed to download " << res.url
                << ": " << res.error << std::endl;
        }
    }
}

std::string HtmlTeXConverter::convert(const std::string& html) const {
    /* fast and optimized precondition checks */
    if (!converter || !valid)
        THROW_RUNTIME_ERROR(
            "HtmlTeXConverter:"
            " Converter not "
            "initialized.", -1);

    /* early return for empty input to avoid unnecessary allocations */
    if (html.empty()) return "";

    /* convert the HTML code to LaTeX */
    char* const raw_result = html2tex_convert(
        converter.get(), 
        html.c_str());

    /* handle nullptr result */
    if (!raw_result) {
        /* distinguish between error and empty conversion */
        if (html2tex_has_error())
            throw LaTeXRuntimeException::fromLaTeXError();

        /* empty result */
        return "";
    }

    /* ensures memory is freed even on exceptions */
    const auto deleter = [](char* p) noexcept { if (p) std::free(p); };
    std::unique_ptr<char[], decltype(deleter)> result_guard(raw_result, deleter);

    /* check if result is actually empty */
    if (raw_result[0] == '\0')
        return "";

    /* measure length first to avoid double calculation */
    const std::size_t result_len = std::char_traits<char>::length(raw_result);

    /* return the result */
    return std::string(raw_result, result_len);
}

bool HtmlTeXConverter::convertToFile(const std::string& html, const std::string& filePath) const {
    /* validate converter and HTML code */
    if (!isValid()) 
        THROW_RUNTIME_ERROR(
            "HtmlTeXConverter: Converter not initialized.", -1);

    if (html.empty()) 
        return false;

    /* convert the HTML code first */
    std::unique_ptr<char[], void(*)(char*)> result(
        html2tex_convert(converter.get(), html.c_str()),
        [](char* p) noexcept { 
            std::free(p); 
        });

    if (!result) {
        if (hasError()) 
            throw std::runtime_error(
                getErrorMessage());
        
        /* empty but valid conversion */
        return false;
    }

    /* write the file */
    std::ofstream fout(filePath);

    if (!fout)
        throw std::runtime_error(
            "Cannot open output file.");

    fout << result.get();
    fout.flush();

    if (!fout)
        throw std::runtime_error(
            "Failed to write LaTeX output.");

    return true;
}

std::string HtmlTeXConverter::convert(const HtmlParser& parser) const {
    /* precondition validation */
    if (!isValid())
        THROW_RUNTIME_ERROR(
            "HtmlTeXConverter in invalid state.", -1);

    /* get HTML content - one serialization only */
    std::string html = parser.toString();

    /* return empty string for empty input */
    if (html.empty()) return "";

    /* perform conversion */
    char* raw_result = html2tex_convert(
        converter.get(), 
        html.c_str());

    /* RAII management with custom deleter */
    const auto deleter = [](char* p) noexcept { std::free(p); };
    std::unique_ptr<char[], decltype(deleter)> result_guard(raw_result, deleter);

    /* error analysis */
    if (!raw_result) {
        if (html2tex_has_error())
            throw LaTeXRuntimeException::fromLaTeXError();

        /* valid empty result */
        return "";
    }

    /* return result (compiler will optimize it) */
    return std::string(raw_result);
}

bool HtmlTeXConverter::convertToFile(const HtmlParser& parser, const std::string& filePath) const {
    /* fast precondition validation */
    if (!converter || !valid)
        THROW_RUNTIME_ERROR(
            "HtmlTeXConverter in invalid state.", -1);

    /* early exit for empty parser */
    if (!parser.hasContent())
        return false;

    /* get serialized HTML - only serialize once */
    const std::string html = parser.toString();

    /* validate serialized content */
    if (html.empty())
        return false;

    /* convert HTML to LaTeX */
    char* raw_result = html2tex_convert(
        converter.get(), 
        html.c_str());

    /* RAII with custom deleter */
    const auto deleter = [](char* p) noexcept {
        if (p) std::free(p);
    };

    std::unique_ptr<char[], decltype(deleter)> result_guard(raw_result, deleter);

    /* check for conversion errors */
    if (!raw_result) {
        if (html2tex_has_error())
            throw LaTeXRuntimeException::fromLaTeXError();

        /* valid empty conversion */
        return false;
    }

    /* check if result is empty */
    if (raw_result[0] == '\0')
        return false;

    /* open file with optimal flags */
    std::ofstream fout;

    /* config stream for maximum performance */
    fout.rdbuf()->pubsetbuf(nullptr, 0);
    fout.open(filePath, std::ios::binary | std::ios::trunc);

    if (!fout)
        throw std::runtime_error(
            "Cannot open output file: "
            + filePath);

    /* get result length once */
    const size_t result_len = std::strlen(raw_result);

    /* write entire result in one operation */
    if (!fout.write(raw_result, static_cast<std::streamsize>(result_len)))
        throw std::runtime_error(
            "Failed to write LaTeX"
            " output to: " 
            + filePath);

    /* explicit flush to ensure data is written */
    fout.flush();

    /* final check */
    if (!fout)
        throw std::runtime_error(
            "Failed to flush LaTeX"
            " output to: " 
            + filePath);

    return true;
}

bool HtmlTeXConverter::convertToFile(const HtmlParser& parser, std::ofstream& output) const {
    /* fast precondition validation */
    if (!converter || !valid)
        THROW_RUNTIME_ERROR(
            "HtmlTeXConverter: Converter not initialized.", -1);

    /* early exit for empty parser */
    if (!parser.hasContent())
        return false;

    /* get serialized HTML - only serialize once */
    const std::string html = parser.toString();

    /* validate serialized content */
    if (html.empty())
        return false;

    /* convert HTML to LaTeX */
    char* raw_result = html2tex_convert(
        converter.get(), 
        html.c_str());

    /* RAII with custom deleter */
    const auto deleter = [](char* p) noexcept {
        /* free handles nullptr */
        std::free(p);
    };

    std::unique_ptr<char[], decltype(deleter)> result_guard(raw_result, deleter);

    /* check for conversion errors */
    if (!raw_result) {
        if (html2tex_has_error())
            throw LaTeXRuntimeException::fromLaTeXError();

        /* valid empty conversion */
        return false;
    }

    /* check if result is empty */
    if (raw_result[0] == '\0')
        return false;

    /* get result length once */
    const std::size_t result_len = std::strlen(raw_result);

    /* write entire result in one operation */
    if (!output.write(raw_result, static_cast<std::streamsize>(result_len)))
        throw std::runtime_error(
            "Failed to write LaTeX"
            " output to stream.");

    /* ensure data is written */
    output.flush();

    if (!output)
        throw std::runtime_error(
            "Failed to flush LaTeX"
            " output to stream.");

    return true;
}

bool HtmlTeXConverter::hasError() const {
    return html2tex_get_error() != HTML2TEX_OK;
}

int HtmlTeXConverter::getErrorCode() const {
    return html2tex_get_error();
}

std::string HtmlTeXConverter::getErrorMessage() const {
    if (!converter) return "Converter not initialized.";
    return html2tex_get_error_message();
}

bool HtmlTeXConverter::isValid() const { return valid; }

HtmlTeXConverter& HtmlTeXConverter::operator =(const HtmlTeXConverter& other) {
    if (this != &other) {
        std::unique_ptr<LaTeXConverter, decltype(&html2tex_destroy)> temp(nullptr, &html2tex_destroy);
        bool new_valid = false;

        if (other.converter && other.valid) {
            LaTeXConverter* clone = html2tex_copy(
                other.converter.get());

            if (clone) {
                downloads_enabled = other.downloads_enabled;
                image_directory = other.image_directory;

                temp.reset(clone);
                new_valid = true;
            }
            else {
                if (html2tex_has_error())
                    throw LaTeXRuntimeException::fromLaTeXError();
            }
        }

        converter = std::move(temp);
        valid = new_valid;
    }

    return *this;
}

HtmlTeXConverter& HtmlTeXConverter::operator =(HtmlTeXConverter&& other) noexcept {
    if (this != &other) {
        downloads_enabled = other.downloads_enabled;
        other.downloads_enabled = false;

        image_directory = other.image_directory;
        other.image_directory = "";

        converter = std::move(other.converter);
        image_manager = std::move(other.image_manager);

        valid = other.valid;
        other.valid = false;
    }

    return *this;
}