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
  bool autoindex;
  size_t client_max_body_size;
  std::string path;
  std::string alias;
  std::string index;
  std::string upload;
  std::map<int, std::string> ret;
  std::set<std::string> limit_except;
  std::map<int, std::string> error_page;
  std::map<std::string, std::string> cgi;
};

struct Server {
  std::vector<std::string> server_names;
  std::vector<Location> locations;
};

typedef std::pair<std::string, uint16_t> ListenerKey;
typedef std::map<ListenerKey, std::vector<Server> > Listeners;

Listeners buildConfig(const std::string &path);
} // namespace config

#endif // !CONFIG_HPP
