#include "Headers.hpp"
#include <algorithm>
#include <cctype>

static const char* const CRLF = "\r\n";
static const size_t CRLF_LEN = 2;

Headers::Headers() {}

std::string Headers::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), static_cast<int(*)(int)>(std::tolower));
    return result;
}

std::string Headers::trim(const std::string& str) {
    if (str.empty()) {
        return str;
    }

    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }

    return str.substr(start, end - start);
}

bool Headers::isTokenChar(char b) {
    if (b >= '0' && b <= '9') {
        return true;
    }
    if (b >= 'A' && b <= 'Z') {
        return true;
    }
    if (b >= 'a' && b <= 'z') {
        return true;
    }
    switch (b) {
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '*':
        case '+':
        case '-':
        case '.':
        case '^':
        case '_':
        case '`':
        case '|':
        case '~':
            return true;
    }

    return false;
}

bool Headers::isToken(const std::string& str) {
    if (str.empty()) {
        return false;
    }
    for (size_t i = 0; i < str.size(); ++i) {
        if (!isTokenChar(str[i])) {
            return false;
        }
    }
    return true;
}

Headers::HeaderParseResult Headers::parseHeader(const std::string& fieldLine) {
    // Find ": " separator
    std::string::size_type separatorPos = fieldLine.find(": ");

    if (separatorPos == std::string::npos) {
        return HeaderParseResult("", "", "malformed field line");
    }

    std::string name = fieldLine.substr(0, separatorPos);
    std::string value = fieldLine.substr(separatorPos + 2);
    value = trim(value);

    // Check if name ends with space
    if (!name.empty() && name[name.size() - 1] == ' ') {
        return HeaderParseResult("", "", "malformed field name");
    }

    return HeaderParseResult(name, value, "");
}

std::string Headers::get(const std::string& name) const {
    std::string lowerName = toLower(name);
    std::map<std::string, std::string>::const_iterator it = headers.find(lowerName);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}

void Headers::set(const std::string& name, const std::string& value) {
    std::string lowerName = toLower(name);
    std::map<std::string, std::string>::iterator it = headers.find(lowerName);
    if (it != headers.end()) {
        it->second = it->second + ", " + value;
    } else {
        headers[lowerName] = value;
    }
}

void Headers::replace(const std::string& name, const std::string& value) {
    std::string lowerName = toLower(name);
    headers[lowerName] = value;
}

void Headers::remove(const std::string& name) {
    std::string lowerName = toLower(name);
    headers.erase(lowerName);
}

Headers::ParseResult Headers::parse(const std::string& data) {
    // Find CRLF
    std::string::size_type idx = data.find(CRLF);

    if (idx == std::string::npos) {
        // No CRLF found, need more data
        return ParseResult(0, false, "");
    }

    // CRLF at start means end of headers
    if (idx == 0) {
        return ParseResult(CRLF_LEN, true, "");
    }

    // Parse single header
    HeaderParseResult headerResult = parseHeader(data.substr(0, idx));
    if (!headerResult.error.empty()) {
        return ParseResult(0, false, headerResult.error);
    }

    if (!isToken(headerResult.name)) {
        return ParseResult(0, false, "malformed header name");
    }

    set(headerResult.name, headerResult.value);

    return ParseResult(static_cast<int>(idx + CRLF_LEN), false, "");
}
