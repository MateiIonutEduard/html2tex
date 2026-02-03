#include "base_exception.hpp"
#include "html2tex.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <cassert>

namespace {

    /**
    * @brief Returns the current timestamp.
    */
    std::string getCurrentTimestamp() {
        std::time_t now = std::time(nullptr);
        char buf[64];

        std::strftime(buf, sizeof(buf),
            "%Y-%m-%d %H:%M:%S",
            std::localtime(&now));

        return buf;
    }

    /**
    * @brief Returns a safe duplicate of specified string.
    */
    std::string safeStrdup(const char* str) {
        return str ? std::string(str) : std::string();
    }

}

struct RuntimeExceptionImpl {
    std::string message;
    int code;
    std::string file;
    int line;
    std::exception_ptr nested;
    std::string timestamp;

    RuntimeExceptionImpl(std::string message, int code, std::string file, int line)
        : message(std::move(message))
        , code(code)
        , file(std::move(file))
        , line(line)
        , nested()
        , timestamp(getCurrentTimestamp()) {
    }

    RuntimeExceptionImpl(const RuntimeExceptionImpl& other)
        : message(other.message)
        , code(other.code)
        , file(other.file)
        , line(other.line)
        , nested(other.nested)
        , timestamp(other.timestamp) {
    }
};

RuntimeException::~RuntimeException() noexcept = default;

RuntimeException::RuntimeException(const std::string& message) noexcept
    : impl(std::make_unique<RuntimeExceptionImpl>(message, 0, "", 0)) {
}

RuntimeException::RuntimeException(const std::string& message, int code) noexcept
    : impl(std::make_unique<RuntimeExceptionImpl>(message, code, "", 0)) {
}

RuntimeException::RuntimeException(const std::string& message, int code,
    const char* file, int line) noexcept
    : impl(std::make_unique<RuntimeExceptionImpl>(message, code, safeStrdup(file), line)) {
}

RuntimeException::RuntimeException(const RuntimeException& other) noexcept
    : impl(std::make_unique<RuntimeExceptionImpl>(*other.impl)) {
}

RuntimeException::RuntimeException(RuntimeException&& other) noexcept
    : impl(std::move(other.impl)) {
}

RuntimeException& RuntimeException::operator=(const RuntimeException& other) noexcept {
    if (this != &other)
        impl = std::make_unique<RuntimeExceptionImpl>(*other.impl);
    return *this;
}

RuntimeException& RuntimeException::operator=(RuntimeException&& other) noexcept {
    if (this != &other)
        impl = std::move(other.impl);
    return *this;
}

const char* RuntimeException::what() const noexcept {
    return impl->message.c_str();
}

std::string RuntimeException::message() const noexcept {
    return impl->message;
}

int RuntimeException::code() const noexcept {
    return impl->code;
}

const char* RuntimeException::file() const noexcept {
    return impl->file.empty() ?
        nullptr : impl->file.c_str();
}

int RuntimeException::line() const noexcept {
    return impl->line;
}

bool RuntimeException::hasError() const noexcept {
    return impl->code != 0;
}

void RuntimeException::setNested(std::exception_ptr nested) noexcept {
    impl->nested = std::move(nested);
}

std::exception_ptr RuntimeException::nested() const noexcept {
    return impl->nested;
}

void RuntimeException::format(std::ostream& stream) const {
    stream << "[Error " << impl->code << "] " << impl->message;

    if (!impl->file.empty()) {
        stream << " (at " << impl->file;

        if (impl->line > 0)
            stream << ":" << impl->line;

        stream << ")";
    }

    stream << " [" << impl->timestamp << "]";
}

std::string RuntimeException::toString() const noexcept {
    std::ostringstream stream;
    format(stream);

    /* append nested exception if present */
    if (impl->nested) {
        try {
            std::rethrow_exception(impl->nested);
        }
        catch (const RuntimeException& e) {
            stream << "\nCaused by: " << e.toString();
        }
        catch (const std::exception& e) {
            stream << "\nCaused by: " << e.what();
        }
        catch (...) {
            stream << "\nCaused by: unknown exception";
        }
    }

    return stream.str();
}

void RuntimeException::throwWithContext(const std::string& message, int code,
    const char* file, int line) {
    throw RuntimeException(message,
        code, file, line);
}

RuntimeException RuntimeException::fromCurrent(const std::string& defaultMessage) {
    const int error_code = html2tex_get_error();
    const char* error_msg = html2tex_get_error_message();

    if (error_code != 0 && error_msg && error_msg[0] != '\0')
        return RuntimeException(
            error_msg,
            error_code);

    return RuntimeException(defaultMessage,
        error_code);
}

RuntimeException::ErrorGuard::ErrorGuard() noexcept
    : saved_error_(html2tex_get_error())
    , saved_message_(html2tex_get_error_message()) {

    /* clear the current error state */
    html2tex_err_clear();
}

RuntimeException::ErrorGuard::~ErrorGuard() noexcept
{ }