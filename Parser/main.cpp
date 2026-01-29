#include "AST.hpp"
#include "Config.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Validator.hpp"
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
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
    validator::validate(http);
    // printBlock(http);
    config::Listeners listeners = config::buildConfig(http);
    for (config::Listeners::iterator l_it = listeners.begin();
         l_it != listeners.end(); ++l_it) {
      std::cout << "address: " << l_it->first.first
                << ", port: " << l_it->first.second << std::endl;

      for (size_t i = 0; i < l_it->second.servers.size(); ++i) {
        std::cout << "\tServer Name: ";
        for (size_t j = 0; j < l_it->second.servers[i].server_name.size(); ++j)
          std::cout << l_it->second.servers[i].server_name[j] << " ";
        std::cout << std::endl;
        for (size_t j = 0; j < l_it->second.servers[i].locations.size(); ++j) {
          config::Location loc = l_it->second.servers[i].locations[j];
          std::cout << "\t"
                    << "Autoindex: " << (loc.autoindex ? "true" : "false")
                    << std::endl;
          std::cout << "\t"
                    << "Index: " << loc.index << std::endl;
          std::cout << "\t"
                    << "Path: '" << loc.path << "'" << std::endl;
          std::cout << "\t"
                    << "Methods:";
          for (std::set<std::string>::iterator m_it = loc.methods.begin();
               m_it != loc.methods.end(); ++m_it)
            std::cout << " '" << *m_it << "'";
          std::cout << std::endl;
          std::cout << "\t"
                    << "Error pages:";
          for (std::map<int, std::string>::iterator e_it =
                   loc.error_pages.begin();
               e_it != loc.error_pages.end(); ++e_it)
            std::cout << " '" << e_it->first << " > " << e_it->second << "'";
          std::cout << std::endl;
        }
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
