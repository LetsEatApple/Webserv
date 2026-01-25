#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "AST.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

namespace config {
struct Location {
  std::string path;
  std::set<std::string> methods;
  std::string root;
  bool autoindex;
  std::string index;
  size_t client_max_body_size;
  std::map<int, std::string> error_pages;
};

struct Server {
  std::vector<std::string> server_name;
  std::vector<Location> locations;
};

typedef std::pair<std::string, uint16_t> ListenerKey;
struct ListenerData {
  std::vector<Server> servers;
  size_t default_index;
};
typedef std::map<ListenerKey, ListenerData> Listeners;

Listeners buildConfig(const Block &http);
} // namespace config

#endif // !CONFIG_HPP
