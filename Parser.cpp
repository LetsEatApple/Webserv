#include "Parser.hpp"
#include "AST.hpp"
#include "Lexer.hpp"
#include "Token.hpp"
#include <stdexcept>
#include <string>
#include <vector>

Parser::Parser(Lexer &lexer) : _lexer(lexer) { advance(); }

// Yield next token
void Parser::advance() { _current = _lexer.next(); }

// Parse everything inside a block: directives and nested blocks
void Parser::parseBlockBody(Block &block) {
  while (_current.type != TOK_RBRACE) {
    if (_current.type != TOK_WORD)
      throw std::runtime_error("Unexpected token inside block (line " +
                               std::to_string(_current.line) + ")");

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
      Directive d;
      d.name = name;
      d.args = args;
      advance(); // consume ';'
      block.directives.push_back(d);
    } else if (_current.type == TOK_LBRACE) {
      // Nested block: recursively parse it
      Block child = parseBlock(name, args);
      block.blocks.push_back(child);
    } else {
      throw std::runtime_error("Expected ';' or '{' inside block (line " +
                               std::to_string(_current.line) + ")");
    }
  }

  advance(); // consume '}'
}

// Create a block with name + args, then parse its body
Block Parser::parseBlock(const std::string &name,
                         const std::vector<std::string> &args) {
  Block block;
  block.name = name;
  block.args = args;

  // Expect opening brace
  if (_current.type != TOK_LBRACE)
    throw std::runtime_error("Expected '{' after block name (line " +
                             std::to_string(_current.line) + ")");
  advance(); // consume '{'

  // Fill block body
  parseBlockBody(block);

  return block;
}

// Creta root block and parse derictives inside it
Block Parser::parse() {
  Block root;
  while (_current.type != TOK_EOF) {
    if (_current.type != TOK_WORD)
      throw std::runtime_error("Unexpected token at top level (line " +
                               std::to_string(_current.line) + ")");

    std::string name = _current.value;
    advance();
    std::vector<std::string> args;
    while (_current.type == TOK_WORD) {
      args.push_back(_current.value);
      advance();
    }
    if (_current.type == TOK_SEMICOLON) {
      Directive d;
      d.name = name;
      d.args = args;
      root.directives.push_back(d);
      advance();
    } else if (_current.type == TOK_LBRACE) {
      Block b = parseBlock(name, args);
      root.blocks.push_back(b);
    } else {
      throw std::runtime_error("Expected ';' or '{' (line " +
                               std::to_string(_current.line) + ")");
    }
  }
  return root;
}
