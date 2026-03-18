#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include "Headers.hpp"

namespace ParserState {
    enum State {
        Init,
        Headers,
        Body,
        Done,
        Error,
        ChunkedSize,
        ChunkedData,
        ChunkedTrailer,
        ChunkedDone
    };
}

struct RequestTarget{
	std::string full;
	std::string path; // should be "/" if none given
	std::string query; // should be "" if none given
};

struct RequestLine {
    std::string method;
    RequestTarget requestTarget;
    std::string httpVersion;
};

class Request {
public:
    Request();

    const std::string&	getMethod() const;
    const std::string&	getTarget() const;
	const std::string&	getPath() const;
	const std::string&	getQuery() const;
	const std::string	getHost() const;
	size_t				getContentLen() const;
    const std::string&	getHttpVersion() const;
    const ::Headers&	getHeaders() const;
    const std::string&	getBody() const;
    std::string			drainBody();
    std::string&		bodyRef();
    void	setMaxBodySize(size_t size);
    void	reset();

	// Check if Content-Type is multipart/form-data
	bool isMultipart(std::string& boundary) const;
	
    template <typename Func>
    void forEachHeader(Func func) const { headers.forEach(func); }

    static const char* const ERROR_MALFORMED_REQUEST_LINE;
    static const char* const ERROR_REQUEST_IN_ERROR_STATE;

    // Parse data incrementally - can be called multiple times
    // Returns number of bytes consumed, or -1 on error
    // Sets errorMsg if provided and an error occurs
    int parse(const std::string& data, std::string& errorMsg);
    int parse(const std::string& data, size_t offset, std::string& errorMsg);

	// Check if parsing header is complete
	bool headersDone() const;
	bool checkState(ParserState::State) const;
    // Check if parsing is complete (either done or error)
    bool done() const;

    // Check if parsing ended in error
    bool error() const;

    // Read and parse a request from a socket (blocking)
    // Returns NULL on error, sets errorMsg if provided
    static Request* requestFromSocket(int socketFd, std::string& errorMsg);



private:
    RequestLine requestLine;
    ::Headers headers;
    std::string body;
    size_t bodyBytesReceived;
    ParserState::State state;
    int chunkedRemaining;
	size_t maxBodySize;

    static const char* const SEPARATOR;

    // Check if request has a body based on Content-Length header
    bool hasBody() const;

    // Check if Transfer-Encoding is chunked
    bool isChunkedEncoding() const;

	bool getBoundary(std::string& boundary) const;

    // Parse request line from buffer
    // Returns bytes consumed (0 if incomplete, -1 on error)
    static int parseRequestLine(const std::string& buffer,
                                RequestLine& rl,
                                std::string& errorMsg);
};

#endif
