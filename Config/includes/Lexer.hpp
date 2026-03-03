#ifndef LEXER_HPP
#define LEXER_HPP

#include "Token.hpp"
#include <cstddef>
#include <fstream>
#include <string>

class Lexer {
public:
  Lexer(const std::string &path);
  token::Token next();

private:
  Lexer(const Lexer &rhs);
  Lexer &operator=(const Lexer &rhs);
  std::ifstream _file;
  size_t _line;

  int get();
  void skipWhitespaceAndComments();
};

#endif // !LEXER_HPP
