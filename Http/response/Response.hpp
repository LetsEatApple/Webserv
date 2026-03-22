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
        StatusRequestTimeout = 408,
        StatusPayloadTooLarge = 413,
        StatusInternalServerError = 500,
        StatusNotImplemented = 501,
        StatusBadGateway = 502,
        StatusGatewayTimeout = 504
    };

    StatusCode intToStatusCode(int code);
    const char* statusCodeToLine(StatusCode code);

    Headers getDefaultHeaders(size_t contentLen);
    std::string getDefaultErrorPage(StatusCode code);

    class Writer {
    public:
        explicit Writer(std::string& buf);
        void writeStatusLine(StatusCode statusCode) const;
        void writeHeaders(const Headers& h) const;
        void writeBody(const char* data, size_t len) const;
        void writeChunkedBody(const char* data, size_t len) const;
        void writeChunkedBodyDone() const;

    private:
        std::string* _buf;
    };

}

#endif
