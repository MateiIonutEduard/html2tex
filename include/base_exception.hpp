#ifndef BASE_EXCEPTION_HPP
#define BASE_EXCEPTION_HPP

#include <exception>
#include <string>
#include <memory>
#include <sstream>
#include <type_traits>
#include <cstddef>
#include <utility>

struct RuntimeExceptionImpl;

/**
 * @brief Base runtime exception class with error code and message support.
 *
 * Provides rich error information including error codes, messages,
 *
 * file/line context, and chained exception support.
 */
class RuntimeException : public std::exception {
public:
    /**
     * @brief Creates a runtime exception with a message.
     * @param message Error description
     */
    explicit RuntimeException(const std::string& message) noexcept;

    /**
     * @brief Creates a runtime exception with message and error code.
     * @param message Error description
     * @param code Numeric error code
     */
    RuntimeException(const std::string& message, int code) noexcept;

    /**
     * @brief Creates a runtime exception with full context.
     * @param message Error description
     * @param code Numeric error code
     * @param file Source file where error occurred
     * @param line Line number where error occurred
     */
    RuntimeException(const std::string& message, int code,
        const char* file, int line) noexcept;

    /**
     * @brief Copy constructor.
     */
    RuntimeException(const RuntimeException&) noexcept;

    /**
     * @brief Move constructor.
     */
    RuntimeException(RuntimeException&&) noexcept;

    /**
     * @brief Destructor.
     */
    virtual ~RuntimeException() noexcept;

    /**
     * @brief Copy assignment operator.
     */
    RuntimeException& operator=(const RuntimeException&) noexcept;

    /**
     * @brief Move assignment operator.
     */
    RuntimeException& operator=(RuntimeException&&) noexcept;

    /**
     * @brief Returns the error message.
     * @return C-style string error message
     */
    virtual const char* what() const noexcept override;

    /**
     * @brief Returns the error message as std::string.
     * @return Error message
     */
    virtual std::string message() const noexcept;

    /**
     * @brief Returns the error code.
     * @return Error code (0 typically means no error)
     */
    virtual int code() const noexcept;

    /**
     * @brief Returns the source file where error occurred.
     * @return File name or empty string if not set
     */
    virtual const char* file() const noexcept;

    /**
     * @brief Returns the line number where error occurred.
     * @return Line number or 0 if not set
     */
    virtual int line() const noexcept;

    /**
     * @brief Checks if error occurred (code != 0).
     * @return True if error occurred, false otherwise
     */
    virtual bool hasError() const noexcept;

    /**
     * @brief Sets a nested exception (chained exception).
     * @param nested Nested exception to chain
     */
    virtual void setNested(std::exception_ptr nested) noexcept;

    /**
     * @brief Gets the nested exception if any.
     * @return Nested exception or null pointer
     */
    virtual std::exception_ptr nested() const noexcept;

    /**
     * @brief Creates a string with full error information.
     * @return Formatted error string with all available context
     */
    virtual std::string toString() const noexcept;

    /**
     * @brief Static method to throw with context.
     * @param message Error description
     * @param code Error code
     * @param file Source file
     * @param line Line number
     */
    static void throwWithContext(const std::string& message, int code = 0,
        const char* file = nullptr, int line = 0);

    /**
     * @brief Static method to create exception from current error state.
     * @param defaultMessage Default message if no error information available
     * @return RuntimeException instance
     */
    static RuntimeException fromCurrent(const std::string& defaultMessage = "Unknown error");

    /**
     * @brief RAII guard for temporarily clearing error state.
     */
    class ErrorGuard {
    public:
        ErrorGuard() noexcept;
        ~ErrorGuard() noexcept;

        ErrorGuard(const ErrorGuard&) = delete;
        ErrorGuard& operator=(const ErrorGuard&) = delete;

    private:
        int saved_error_;
        std::string saved_message_;
    };

protected:
    /**
     * @brief Virtual method for custom string formatting.
     * @param stream Output stream to format into
     */
    virtual void format(std::ostream& stream) const;

private:
    std::unique_ptr<RuntimeExceptionImpl> impl;
};

/**
 * @brief Macro for throwing RuntimeException exceptions with file/line context.
 */
#define THROW_RUNTIME_ERROR(msg, code) \
    RuntimeException::throwWithContext((msg), (code), __FILE__, __LINE__)

 /**
  * @brief Macro for throwing if condition is true.
  */
#define THROW_IF(condition, msg, code) \
    do { \
        if ((condition)) { \
            THROW_RUNTIME_ERROR((msg), (code)); \
        } \
    } while (false)

#endif