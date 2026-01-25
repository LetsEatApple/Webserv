#include "Parser.hpp"
#include "AST.hpp"
#include "Lexer.hpp"
#include "Token.hpp"
#include "Utils.hpp"
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

Parser::Parser(Lexer &lexer) : _lexer(lexer) { advance(); }

// Yield next token
void Parser::advance() { _current = _lexer.next(); }

// Parse everything inside a block: directives and nested blocks
void Parser::parseBlockBody(Block &block) {
  std::stringstream err;
  while (_current.type != TOK_RBRACE) {
    if (_current.type != TOK_WORD)
      throw std::runtime_error(
          utils::errorMsg("Unexpected token inside block", _current.line));

    // Read entry name
    std::string name = _current.value;
    advance(); // consume name

    // Read entry arguments
    std::vector<std::string> args;
    while (_current.type == TOK_WORD) {
      args.push_back(_current.value);
      advance();
    }

    // Decide if it's a directive or nested block
    if (_current.type == TOK_SEMICOLON) {
      Directive d = {name, args, _current.line};
      advance(); // consume ';'
      block.directives.push_back(d);
    } else if (_current.type == TOK_LBRACE) {
      // Nested block: recursively parse it
      Block child = parseBlock(name, args);
      block.blocks.push_back(child);
    } else
      throw std::runtime_error(
          utils::errorMsg("Expected ';' or '{' inside block", _current.line));
  }

  advance(); // consume '}'
}

// Create a block with name + args, then parse its body
Block Parser::parseBlock(const std::string &name,
                         const std::vector<std::string> &args) {
  std::stringstream err;

  Block block = {name, args, std::vector<Directive>(), std::vector<Block>(),
                 _current.line};

  // Expect opening brace
  if (_current.type != TOK_LBRACE)
    throw std::runtime_error(
        utils::errorMsg("Expected '{' after block name", _current.line));
  advance(); // consume '{'

  // Fill block body
  parseBlockBody(block);

  return block;
}

// Create root 'http' block and parse derictives inside it
Block Parser::parse() {
  std::stringstream err;

  Block http = {"http", std::vector<std::string>(), std::vector<Directive>(),
                std::vector<Block>(), _current.line};
  while (_current.type != TOK_EOF) {
    if (_current.type != TOK_WORD)
      throw std::runtime_error(
          utils::errorMsg("Unexpected token at top level", _current.line));
    std::string name = _current.value;
    size_t line = _current.line;
    advance();
    std::vector<std::string> args;
    while (_current.type == TOK_WORD) {
      args.push_back(_current.value);
      advance();
    }
    if (_current.type == TOK_SEMICOLON) {
      Directive d = {name, args, line};
      http.directives.push_back(d);
      advance();
    } else if (_current.type == TOK_LBRACE) {
      Block b = parseBlock(name, args);
      b.line = line;
      http.blocks.push_back(b);
    } else
      throw std::runtime_error(
          utils::errorMsg("Expected ';' or '{'", _current.line));
  }
  return http;
}
