#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Headers.hpp"

namespace Response {

    enum StatusCode {
        StatusOk = 200,
        StatusNoContent = 204,
        StatusMovedPermanently = 301,
        StatusFound = 302,
        StatusTemporaryRedirect = 307,
        StatusPermanentRedirect = 308,
        StatusBadRequest = 400,
        StatusForbidden = 403,
        StatusNotFound = 404,
        StatusMethodNotAllowed = 405,
        StatusPayloadTooLarge = 413,
        StatusInternalServerError = 500,
        StatusNotImplemented = 501
    };

    StatusCode intToStatusCode(int code);
    const char* statusCodeToLine(StatusCode code);

    Headers getDefaultHeaders(int contentLen);
    std::string getDefaultErrorPage(StatusCode code);

    class Writer {
    public:
        Writer(std::string& buf);
        bool writeStatusLine(StatusCode statusCode) const;
        bool writeHeaders(const Headers& h) const;
        bool writeBody(const char* data, size_t len) const;
        bool writeChunkedBody(const char* data, size_t len) const;
        bool writeChunkedBodyDone() const;

    private:
        std::string* _buf;
    };

}

#endif
