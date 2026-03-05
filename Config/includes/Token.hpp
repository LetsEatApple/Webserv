#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

namespace token
{
    enum TokenType
    {
        TOK_WORD,
        TOK_LBRACE,
        TOK_RBRACE,
        TOK_SEMICOLON,
        TOK_EOF
    };

    struct Token
    {
        TokenType type;
        std::string value;
        std::size_t line;
    };
}

#endif //! TOKEN_HPP
