#include "AST.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Validator.hpp"
#include <cstddef>
#include <exception>
#include <iostream>
#include <vector>

void printBlock(const Block &block, int indent = 0) {
  std::string space(indent, ' ');

  std::cout << space << "Block: " << block.name;
  for (size_t i = 0; i < block.args.size(); ++i)
    std::cout << " " << block.args[i];
  std::cout << std::endl;

  for (size_t i = 0; i < block.directives.size(); ++i) {
    std::cout << space << "  Directive: " << block.directives[i].name;
    std::cout << " args: [";
    for (size_t j = 0; j < block.directives[i].args.size(); ++j)
      std::cout << " " << block.directives[i].args[j];
    std::cout << " ]" << std::endl;
  }

  for (size_t i = 0; i < block.blocks.size(); ++i)
    printBlock(block.blocks[i], indent + 2);
}
int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " config.conf" << std::endl;
    return 1;
  }

  try {
    Lexer lexer(argv[1]);
    Parser parser(lexer);
    Block http = parser.parse();
    Validator::validate(http);
    printBlock(http);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
