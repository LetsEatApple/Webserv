#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>

/**
 * @struct Directive
 * @brief Represents a single directive in the AST.
 *
 * A simple directive consists of the name and parameters separated
 * by spaces and ends with a semicolon (;).
 *
 * @note root /data/www;
 */
struct Directive {
  /// Name of the directive
  std::string name;

  /// List of arguments passed to the directive
  std::vector<std::string> args;

  /// Line number in the configuration file where the directive occurs
  size_t line;
};

/**
 * @struct Block
 * @brief Represents a block in the AST, which may contain directives and nested
 * blocks.
 *
 * A block directive has the same structure as a simple directive,
 * but instead of the semicolon it ends with a set of additional instructions
 * surrounded by braces ({ and }).
 */
struct Block {
  /// Name of the block
  /// @note "server", "location"
  std::string name;

  /// List of arguments associated with the block
  std::vector<std::string> args;

  /// Directives that are directly inside this block
  std::vector<Directive> directives;

  /// Nested blocks inside this block
  std::vector<Block> blocks;

  /// Line number in the configuration file where the block starts
  size_t line;
};

#endif // !AST_HPP
