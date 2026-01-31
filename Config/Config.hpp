#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstddef>
#include <map>
#include <ostream>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

namespace config {
struct Location {
  std::string path;
  std::set<std::string> limit_except;
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
typedef std::map<ListenerKey, std::vector<Server> > Listeners;

Listeners buildConfig(const std::string &path);
std::ostream &operator<<(std::ostream &out, Location const &l);
std::ostream &operator<<(std::ostream &out, Server const &l);
std::ostream &operator<<(std::ostream &out, Listeners const &l);
} // namespace config

#endif // !CONFIG_HPP
