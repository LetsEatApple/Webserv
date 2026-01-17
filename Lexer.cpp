#include "Lexer.hpp"
#include "Token.hpp"
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

Lexer::Lexer(const std::string &path) : _file(path.c_str()), _line(1) {
  if (!_file.is_open())
    throw std::runtime_error("Failed to open configuration file");
}

int Lexer::get() {
  int c = _file.get();
  if (c == '\n')
    _line++;
  return c;
}

void Lexer::skipWhitespaceAndComments() {
  int c;
  for (;;) {
    c = get();
    if (c == EOF)
      return;
    if (std::isspace(c) || c == '\n')
      continue;
    if (c == '#') {
      while ((c = get()) != EOF && c != '\n') {
      }
      continue;
    }
    _file.unget();
    return;
  }
}

Token Lexer::next() {
  skipWhitespaceAndComments();

  int c = get();
  if (c == EOF)
    return Token(TOK_EOF, "", _line);

  if (c == '{')
    return Token(TOK_LBRACE, "{", _line);
  if (c == '}')
    return Token(TOK_RBRACE, "}", _line);
  if (c == ';')
    return Token(TOK_SEMICOLON, ";", _line);

  std::string word;
  if (c == '"' || c == '\'') {
    int quote = c;
    while ((c = get()) != quote) {
      if (c == EOF)
        throw std::runtime_error("Unterminated string");
      word += c;
    }
    return Token(TOK_WORD, word, _line);
  }

  while (c != EOF && !std::isspace(c) && c != '{' && c != '}' && c != ';') {
    word += c;
    c = get();
  }

  if (c == '{' || c == '}' || c == ';')
    _file.unget();

  return Token(TOK_WORD, word, _line);
}
