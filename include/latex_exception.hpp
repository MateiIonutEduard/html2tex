#ifndef LATEX_EXCEPTION_HPP
#define LATEX_EXCEPTION_HPP

#include "base_exception.hpp"

/**
 * @brief LaTeX conversion-specific runtime exception.
 */
class LaTeXRuntimeException : public RuntimeException {
public:
    explicit LaTeXRuntimeException(const std::string& message) noexcept;
    LaTeXRuntimeException(const std::string& message, int code) noexcept;
    LaTeXRuntimeException(const std::string& message, int code,
        const char* file, int line) noexcept;

    virtual ~LaTeXRuntimeException() noexcept override = default;

    /**
     * @brief Creates exception from LaTeX conversion error.
     * @return LaTeXRuntimeException with current error state
     */
    static LaTeXRuntimeException fromLaTeXError();

protected:
    virtual void format(std::ostream& stream) const override;
};

/**
 * @brief Macro for throwing LaTeXRuntimeException exceptions with file/line context.
 */
#define THROW_LATEX_ERROR(msg, code) \
    throw LaTeXRuntimeException((msg), (code), __FILE__, __LINE__)

#endif