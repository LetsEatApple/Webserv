#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>

struct Directive {
  std::string name;
  std::vector<std::string> args;
  size_t line;
};

struct Block {
  std::string name;
  std::vector<std::string> args;
  std::vector<Directive> directives;
  std::vector<Block> blocks;
  size_t line;
};

#endif // !AST_HPP
