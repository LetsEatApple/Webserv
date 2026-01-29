#include "Config.hpp"
#include "AST.hpp"
#include "Utils.hpp"
#include <cstddef>

#include <cstdlib>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

namespace config {

std::pair<std::string, uint16_t> parseListen(Directive const &d) {
  std::string arg = d.args[0];
  std::pair<std::string, uint16_t> listen;
  std::string host = "0.0.0.0";
  uint16_t port = 8000;
  size_t colon_pos = arg.find(":");
  if (colon_pos != std::string::npos) {
    host = arg.substr(0, colon_pos);
    std::string portStr = arg.substr(colon_pos + 1);
    if (!utils::isNumber(portStr))
      throw std::runtime_error(utils::errorMsg(
          "Invalid port in '" + arg + "' of the 'listen' directive", d.line));
    if (host == "*")
      host = "0.0.0.0";
    long p = std::strtol(portStr.c_str(), NULL, 10);
    if (p < 1 || p > 65535)
      throw std::runtime_error(utils::errorMsg(
          "Invalid port in '" + arg + "' of the 'listen' directive", d.line));
    port = static_cast<uint16_t>(p);
  }
  listen.first = host;
  listen.second = port;
  return listen;
}

void applyDirective(Location &server_default_location, Directive const &d) {
  (void)server_default_location;
  (void)d;
}

Location buildLocation(const Block &b, Location default_location) {
  (void)b;
  return default_location;
}

bool hasRootLocation(std::vector<Location> &locations) {
  for (size_t i = 0; i < locations.size(); ++i) {
    if (locations[i].path == "/")
      return true;
  }
  return false;
}

void buildServers(Listeners &listeners, const Block &server,
                  Location http_default_location) {
  Location server_default_location = http_default_location;
  std::vector<ListenerKey> listens;
  std::vector<std::string> server_name;

  for (size_t i = 0; i < server.directives.size(); ++i) {
    const Directive &d = server.directives[i];

    if (d.name == "listen")
      listens.push_back(parseListen(d));
    else if (d.name == "server_name")
      server_name = d.args;
    else
      applyDirective(server_default_location, d);
  }

  for (size_t i = 0; i < listens.size(); ++i) {
    Server s;
    s.server_name = server_name;

    for (size_t j = 0; j < server.blocks.size(); ++j) {
      Location loc = buildLocation(server.blocks[j], server_default_location);
      s.locations.push_back(loc);
    }
    if (!hasRootLocation(s.locations))
      s.locations.push_back(server_default_location);
    listeners[listens[i]].servers.push_back(s);
  }
}

Listeners buildConfig(const Block &http) {
  Listeners listeners;

  Location http_default_location;
  http_default_location.path = "/";
  http_default_location.root = "/var/www";
  http_default_location.index = "index.html";
  http_default_location.client_max_body_size = 1024 * 1024;
  http_default_location.error_pages = std::map<int, std::string>();
  std::set<std::string> methods;
  methods.insert("GET");
  http_default_location.methods = methods;

  for (size_t i = 0; i < http.blocks.size(); ++i)
    buildServers(listeners, http.blocks[i], http_default_location);
  return listeners;
}
}; // namespace config
