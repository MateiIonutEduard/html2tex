#ifndef HTMLTEX_EXCEPTION_HPP
#define HTMLTEX_EXCEPTION_HPP
#include "base_exception.hpp"

/**
 * @brief Html-specific runtime exception.
 */
class HtmlRuntimeException : public RuntimeException {
public:
    explicit HtmlRuntimeException(const std::string& message) noexcept;
    HtmlRuntimeException(const std::string& message, int code) noexcept;
    HtmlRuntimeException(const std::string& message, int code,
        const char* file, int line) noexcept;

    virtual ~HtmlRuntimeException() noexcept override = default;

    /**
     * @brief Creates exception from HTML parsing error.
     * @return HtmlRuntimeException with current error state
     */
    static HtmlRuntimeException fromHtmlError();

protected:
    virtual void format(std::ostream& stream) const override;
};

/**
 * @brief Macro for throwing HtmlRuntimeException exceptions with file/line context.
 */
#define THROW_HTML_ERROR(msg, code) \
    throw HtmlRuntimeException((msg), (code), __FILE__, __LINE__)

#endif