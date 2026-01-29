#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstddef>
#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

namespace config {
struct Location {
  std::string path;
  std::set<std::string> limit_exept;
  std::string alias;
  bool autoindex;
  std::string index;
  size_t client_max_body_size;
  std::map<int, std::string> returns;
  std::map<int, std::string> error_pages;
  std::string upload;
};

struct Server {
  std::vector<std::string> server_names;
  std::vector<Location> locations;
};

typedef std::pair<std::string, uint16_t> ListenerKey;
struct ListenerData {
  std::vector<Server> servers;
  size_t default_index;
};
typedef std::map<ListenerKey, ListenerData> Listeners;

Listeners buildConfig(const std::string &path);
} // namespace config

#endif // !CONFIG_HPP
