#include "Config/Config.hpp"
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " config.conf" << std::endl;
    return 1;
  }

  try {
    config::Listeners listeners = config::buildConfig(argv[1]);
    for (config::Listeners::iterator l_it = listeners.begin();
         l_it != listeners.end(); ++l_it) {
      std::cout << "address: " << l_it->first.first
                << ", port: " << l_it->first.second << std::endl;

      for (size_t i = 0; i < l_it->second.servers.size(); ++i) {
        std::cout << "\tServer Name: ";
        for (size_t j = 0; j < l_it->second.servers[i].server_names.size(); ++j)
          std::cout << l_it->second.servers[i].server_names[j] << " ";
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
          for (std::set<std::string>::iterator m_it = loc.limit_exept.begin();
               m_it != loc.limit_exept.end(); ++m_it)
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
