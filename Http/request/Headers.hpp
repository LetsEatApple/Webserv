#ifndef HEADERS_HPP
#define HEADERS_HPP

#include <string>
#include <map>

class Headers {
public:
    Headers();

    std::string get(const std::string& name) const;
    void set(const std::string& name, const std::string& value);
    void replace(const std::string& name, const std::string& value);
    void remove(const std::string& name);
    template <typename Func>
    void forEach(Func func) const {
        for (std::map<std::string, std::string>::const_iterator it = headers.begin();
             it != headers.end(); ++it) {
            func(it->first, it->second);
        }
    }

    // Returns: bytes consumed, done flag, error message (empty if no error)
    struct ParseResult {
        int bytesConsumed;
        bool done;
        std::string error;

        ParseResult() : bytesConsumed(0), done(false) {}
        ParseResult(int b, bool d, const std::string& e) : bytesConsumed(b), done(d), error(e) {}
    };

    ParseResult parse(const std::string& data);

private:
    std::map<std::string, std::string> headers;

    static std::string toLower(const std::string& str);
    static std::string trim(const std::string& str);
    static bool isToken(const std::string& str);
    static bool isTokenChar(char b);

    struct HeaderParseResult {
        std::string name;
        std::string value;
        std::string error;

        HeaderParseResult() {}
        HeaderParseResult(const std::string& n, const std::string& v, const std::string& e)
            : name(n), value(v), error(e) {}
    };

    static HeaderParseResult parseHeader(const std::string& fieldLine);
};

#endif // HEADERS_HPP
