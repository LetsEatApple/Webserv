#include "Request.hpp"
#include <sys/socket.h>
#include <cstring>
#include <cstdlib>

const char* const Request::ERROR_MALFORMED_REQUEST_LINE = "malformed request-line";
const char* const Request::ERROR_REQUEST_IN_ERROR_STATE = "request in error state";
const char* const Request::SEPARATOR = "\r\n";

Request::Request() : bodyBytesReceived(0), state(ParserState::Init), chunkedRemaining(0), maxBodySize(0) {}

void Request::reset() {
	requestLine.method.clear();
	requestLine.requestTarget.full.clear();
	requestLine.requestTarget.path.clear();
	requestLine.requestTarget.query.clear();
	requestLine.httpVersion.clear();
	headers.clear();
	body.clear();
	bodyBytesReceived = 0;
	state = ParserState::Init;
	chunkedRemaining = 0;
	maxBodySize = 0;
}

const std::string& Request::getMethod() const {
    return requestLine.method;
}

const std::string& Request::getTarget() const {
    return requestLine.requestTarget.full;
}

const std::string&	Request::getPath() const{
	return requestLine.requestTarget.path;
}

const std::string&	Request::getQuery() const{
	return requestLine.requestTarget.query;
}

const std::string Request::getHost() const {
	return headers.get("host");
}

size_t Request::getContentLen() const {
	std::string lengthStr = headers.get("content-length");
	if (lengthStr.empty())
		return 0;

	int length = std::atoi(lengthStr.c_str());
	return length;
}

const std::string& Request::getHttpVersion() const {
    return requestLine.httpVersion;
}

const Headers& Request::getHeaders() const {
    return headers;
}

const std::string& Request::getBody() const {
    return body;
}

std::string Request::drainBody() {
    std::string tmp = body;
    body.clear();
    return tmp;
}

std::string& Request::bodyRef() {
    return body;
}

bool Request::hasBody() const {
    std::string lengthStr = headers.get("content-length");
    if (lengthStr.empty()) {
        return false;
    }
    int length = std::atoi(lengthStr.c_str());
    return length > 0;
}

bool Request::isChunkedEncoding() const {
    return headers.get("transfer-encoding") == "chunked";
}

bool Request::headersDone() const{
	return (state > ParserState::Headers);
}

bool Request::checkState(ParserState::State arg) const{
	return (state == arg);
}

bool Request::done() const {
    return state == ParserState::Done || state == ParserState::Error;
}

bool Request::error() const {
    return state == ParserState::Error;
}

/*** include check origin-form for request target ***/
// only path (with leading slash) & one query allowed
// not allowed: fragments, absoluteURI, authority forms, multiple queries
bool originForm(RequestTarget *rt){

	rt->query.clear();
	rt->path.clear();

	if (rt->full.rfind("/", 0) != 0)
		return false;
	if (rt->full.find("://", 0) != std::string::npos)
		return false;

	if (rt->full.find("#") != std::string::npos)
		return false;
		
	size_t q = rt->full.find("?");
	if (q == std::string::npos)
		rt->path = rt->full;
	else {
		if (rt->full.find('?', q+1) != std::string::npos)
			return false;
		rt->path = rt->full.substr(0, q);
		rt->query = rt->full.substr(q+1);
	}
	return true;
}

int Request::parseRequestLine(const std::string& buffer,
                              RequestLine& rl,
                              std::string& errorMsg) {
    std::string::size_type idx = buffer.find(SEPARATOR);
    if (idx == std::string::npos) {
        return 0; // Need more data
    }

    std::string startLine = buffer.substr(0, idx);
    int bytesRead = static_cast<int>(idx) + 2; // Include \r\n

    // Split by spaces - need exactly 3 parts
    std::string::size_type firstSpace = startLine.find(' ');
    if (firstSpace == std::string::npos) {
        errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }

    std::string::size_type secondSpace = startLine.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos) {
        errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }

    // Check no more spaces after secondSpace
    if (startLine.find(' ', secondSpace + 1) != std::string::npos) {
        errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }

    rl.method = startLine.substr(0, firstSpace);
    rl.requestTarget.full = startLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    std::string httpVersionFull = startLine.substr(secondSpace + 1);

    if (!originForm(&rl.requestTarget)){
        errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }
	
    // Validate HTTP version format: HTTP/1.1
    std::string::size_type slashPos = httpVersionFull.find('/');
    if (slashPos == std::string::npos) {
        errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }

    std::string httpPrefix = httpVersionFull.substr(0, slashPos);
    std::string httpVersion = httpVersionFull.substr(slashPos + 1);

    if (httpPrefix != "HTTP" || httpVersion != "1.1") {
        errorMsg = ERROR_MALFORMED_REQUEST_LINE;
        return -1;
    }

    rl.httpVersion = httpVersion;
    return bytesRead;
}

int Request::parse(const std::string& data, size_t offset, std::string& errorMsg) {
    int totalRead = 0;
    size_t pos = offset;
    const size_t dataSize = data.size();

    while (true) {
        size_t remaining = dataSize - pos;
        switch (state) {
            case ParserState::Error:
                errorMsg = ERROR_REQUEST_IN_ERROR_STATE;
                return -1;

            case ParserState::Init: {
                std::string currentData = data.substr(pos);
                int n = parseRequestLine(currentData, requestLine, errorMsg);
                if (n < 0) {
                    state = ParserState::Error;
                    return -1;
                }
                if (n == 0) {
                    return 0; // Need more data
                }
                totalRead += n;
                pos += n;
                state = ParserState::Headers;
                break;
            }

            case ParserState::Headers: {
                std::string currentData = data.substr(pos);
                ::Headers::ParseResult result = headers.parse(currentData);
                if (!result.error.empty()) {
                    errorMsg = result.error;
                    state = ParserState::Error;
                    return -1;
                }
                if (result.bytesConsumed == 0) {
                    return totalRead; // Need more data
                }
                totalRead += result.bytesConsumed;
                pos += result.bytesConsumed;
                if (result.done) { // at this point headers are complete
					std::string clStr = headers.get("content-length");
					if (!clStr.empty() && std::atoi(clStr.c_str()) < 0) {
						errorMsg = "invalid content-length";
						state = ParserState::Error;
						return -1;
					}
					if (isChunkedEncoding()) {
                        state = ParserState::ChunkedSize;
                    } else if (hasBody()) {
                        state = ParserState::Body;
                    } else {
                        state = ParserState::Done;
                    }
					return totalRead;
                }
                break;
            }

            case ParserState::Body: {
                size_t length = static_cast<size_t>(std::atoi(headers.get("content-length").c_str()));
                size_t bodyRemaining = length - bodyBytesReceived;
                size_t toConsume = (bodyRemaining < remaining) ? bodyRemaining : remaining;
                body.append(data, pos, toConsume);
                totalRead += static_cast<int>(toConsume);
                pos += toConsume;
                bodyBytesReceived += toConsume;

                if (bodyBytesReceived == length) {
                    state = ParserState::Done;
                }
                return totalRead;
            }

            case ParserState::ChunkedSize: {
                std::string::size_type idx = data.find(SEPARATOR, pos);
                if (idx == std::string::npos) {
                    return totalRead; // Need more data
                }
                size_t len = idx - pos;
                std::string sizeStr = data.substr(pos, len);
                char* end = NULL;
                unsigned long chunkSize = std::strtoul(sizeStr.c_str(), &end, 16);
                if (end == sizeStr.c_str() || *end != '\0') {
                    errorMsg = "invalid chunk size";
                    state = ParserState::Error;
                    return -1;
                }
                int consumed = static_cast<int>(len) + 2;
                totalRead += consumed;
                pos += consumed;
                if (chunkSize == 0) {
                    state = ParserState::ChunkedDone;
                } else if (maxBodySize > 0 && bodyBytesReceived + chunkSize > maxBodySize) {
                    errorMsg = "Payload Too Large";
                    state = ParserState::Error;
                    return -1;
                } else {
                    chunkedRemaining = static_cast<int>(chunkSize);
                    state = ParserState::ChunkedData;
                }
                break;
            }

            case ParserState::ChunkedData: {
                if (remaining == 0) {
                    return totalRead; // Need more data
                }
                int available = static_cast<int>(remaining);
                int toConsume = (chunkedRemaining < available) ? chunkedRemaining : available;
                body.append(data, pos, toConsume);
                totalRead += toConsume;
                pos += toConsume;
                chunkedRemaining -= toConsume;
                bodyBytesReceived += static_cast<size_t>(toConsume);
                if (chunkedRemaining == 0) {
                    state = ParserState::ChunkedTrailer;
                }
                break;
            }

            case ParserState::ChunkedTrailer: {
                if (remaining < 2) {
                    return totalRead; // Need more data
                }
                totalRead += 2; // consume \r\n after chunk data
                pos += 2;
                state = ParserState::ChunkedSize;
                break;
            }

            case ParserState::ChunkedDone: {
                if (remaining < 2) {
                    return totalRead; // Need more data
                }
                totalRead += 2; // consume final \r\n
                pos += 2;
                state = ParserState::Done;
                break;
            }

            case ParserState::Done:
                return totalRead;
        }
    }
}

int Request::parse(const std::string& data, std::string& errorMsg) {
    return parse(data, 0, errorMsg);
}

Request* Request::requestFromSocket(int socketFd, std::string& errorMsg) {
    Request* request = new Request();

    char buf[1024];
    int bufLen = 0;

    while (!request->done()) {
        ssize_t n = recv(socketFd, buf + bufLen, sizeof(buf) - bufLen, 0);
        if (n < 0) {
            errorMsg = "socket read error";
            delete request;
            return NULL;
        }
        if (n == 0) {
            errorMsg = "connection closed";
            delete request;
            return NULL;
        }

        bufLen += n;

        std::string data(buf, bufLen);
        int readN = request->parse(data, errorMsg);
        if (readN < 0) {
            delete request;
            return NULL;
        }

        // Shift buffer to remove parsed data
        std::memmove(buf, buf + readN, bufLen - readN);
        bufLen -= readN;
    }

    return request;
}

void	Request::setMaxBodySize(size_t size){
	this->maxBodySize = size;
}
