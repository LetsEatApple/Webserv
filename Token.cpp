#include "Token.hpp"
#include <cstddef>
#include <string>

Token::Token() {}
Token::Token(TokenType type, std::string value, size_t line)
    : type(type), value(value), line(line) {}
