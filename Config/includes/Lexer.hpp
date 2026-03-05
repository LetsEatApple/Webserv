#ifndef LEXER_HPP
#define LEXER_HPP

#include <fstream>
#include <string>
#include "Token.hpp"

class Lexer
{
public:
    explicit Lexer(const std::string& path);
    token::Token next();

private:
    Lexer(const Lexer& rhs);
    Lexer& operator=(const Lexer& rhs);
    std::ifstream _file;
    size_t _line;

    int get();
    void skipWhitespaceAndComments();
};

#endif // !LEXER_HPP
