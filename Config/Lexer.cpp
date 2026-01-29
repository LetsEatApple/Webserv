#include "Lexer.hpp"
#include "Token.hpp"
#include "Utils.hpp"
#include <cctype>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

Lexer::Lexer(const std::string &path) : _file(path.c_str()), _line(1) {
  if (!_file.is_open())
    throw std::runtime_error("Failed to open configuration file at '" + path +
                             "'");
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
  if (c == EOF) {
    Token tok = {TOK_EOF, "", _line};
    return tok;
  }

  if (c == '{') {
    Token tok = {TOK_LBRACE, "{", _line};
    return tok;
  }

  if (c == '}') {
    Token tok = {TOK_RBRACE, "}", _line};
    return tok;
  }
  if (c == ';') {
    Token tok = {TOK_SEMICOLON, ";", _line};
    return tok;
  }

  std::string word;
  if (c == '"' || c == '\'') {
    int quote = c;
    while ((c = get()) != quote) {
      if (c == EOF)
        throw std::runtime_error(utils::errorMsg("Unterminated string", _line));
      word += c;
    }
    Token tok = {TOK_WORD, word, _line};
    return tok;
  }

  while (c != EOF && !std::isspace(c) && c != '{' && c != '}' && c != ';') {
    word += c;
    c = get();
  }

  if (c == '{' || c == '}' || c == ';')
    _file.unget();

  Token tok = {TOK_WORD, word, _line};
  return tok;
}
