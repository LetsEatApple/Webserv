
#include "AST.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include <string>

static Block parseFile(std::string const &path) {
  Lexer lexer(path);
  Parser parser(lexer);
  return parser.parse();
}
