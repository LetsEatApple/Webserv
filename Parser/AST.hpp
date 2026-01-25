#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>

struct Directive {
  std::string name;
  std::vector<std::string> args;
};

struct Block {
  std::string name;
  std::vector<std::string> args;
  std::vector<Directive> directives;
  std::vector<Block> blocks;
};

#endif // !AST_HPP
