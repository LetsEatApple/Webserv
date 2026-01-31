#include "Config.hpp"
#include "AST.hpp"
#include "Debug.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Utils.hpp"
#include "Validator.hpp"
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <netdb.h>
#include <ostream>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <utility>
#include <vector>

namespace config {

static bool isBracketedIPv6(const std::string &s) {
  bool onlyAddress = s.size() >= 2 && s[0] == '[' && s[s.size() - 1] == ']';
  size_t last_close_pos = s.find_last_of("]");
  bool addressAndPort = s.size() >= 2 && s[0] == '[' &&
                        last_close_pos != std::string::npos &&
                        last_close_pos != s.size() + 1 && s[last_close_pos + 1] == ':';
  return onlyAddress || addressAndPort;
}

static std::string getAddress(std::string &address, std::string &port,
                              Directive const &d, bool isV6) {
  if (address == "*")
    address = "0.0.0.0";
  if (address == "localhost")
    address = "127.0.0.1";

  struct addrinfo hints;
  struct addrinfo *res;
  if (isV6)
    hints.ai_family = AF_INET6;
  else
   hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_addrlen = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  hints.ai_flags = 0;
  if (getaddrinfo(address.c_str(), port.c_str(), &hints, &res) != 0)
    throw std::runtime_error(utils::errorMsg(
        "Invalid address '" + address + "' of the 'listen' directive", d.line));
  freeaddrinfo(res);
  return address;
}

static uint16_t getPort(std::string const &portStr, Directive const &d) {
  if (!utils::isNumber(portStr))
    throw std::runtime_error(utils::errorMsg("Invalid port in '" + d.args[0] +
                                                 "' of the 'listen' directive",
                                             d.line));
  long p = std::strtol(portStr.c_str(), NULL, 10);
  if (p < 1 || p > 65535)
    throw std::runtime_error(utils::errorMsg("Invalid port in '" + d.args[0] +
                                                 "' of the 'listen' directive",
                                             d.line));
  return static_cast<uint16_t>(p);
}

std::pair<std::string, uint16_t> makeListen(Directive const &d) {
  std::string arg = d.args[0];
  std::pair<std::string, uint16_t> listen;

  std::string address = "0.0.0.0";
  uint16_t port = 8000;

  /* IPv6 [addr]:port */
  if (isBracketedIPv6(arg)) {
    size_t close = arg.find(']');
    address = arg.substr(1, close - 1);

    if (close + 1 < arg.size()) {
      if (arg[close + 1] != ':')
        throw std::runtime_error(
            utils::errorMsg("Invalid IPv6 address format in '" + arg +
                                "' of the 'listen' directive",
                            d.line));

      std::string portStr = arg.substr(close + 2);
      port = getPort(portStr, d);
    }
    std::string portStr = utils::toString(port);
    address = getAddress(address, portStr, d, true);
  }
  /* Ipv6 / hostname */
  else {
    size_t colon_pos = arg.find_last_of(":");

    if (colon_pos != std::string::npos) {
      address = arg.substr(0, colon_pos);
      std::string portStr = arg.substr(colon_pos + 1);
      port = getPort(portStr, d);
      address = getAddress(address, portStr, d, false);
    } else if (utils::isNumber(arg))
      port = getPort(arg, d);
    else {
      std::string partStr = utils::toString(port);
      address = getAddress(arg, partStr, d, false);
    }
  }

  listen.first = address;
  listen.second = port;
  return listen;
}

static void  applyLocationPath(Location &loc, Block const &b) {
  std::string const &path = b.args.at(0);

  if (path.empty() || path.at(0) != '/')
    throw std::runtime_error(utils::errorMsg("Unexpected value '" + path + "' in 'location' directive", b.line));

  std::string normalized = path;
  if (normalized.size() > 1 && normalized.back() == '/')
    normalized.pop_back();
  loc.path = normalized;
}

static void  applyAlias(Location &loc, Directive const &d) {
  std::string const &alias = d.args.at(0);

  if (alias.empty() || alias.at(0) != '/')
    throw std::runtime_error(utils::errorMsg("Unexpected value '" + alias +  "' in 'alias' directive", d.line));

  std::string normalized = alias;
  if (normalized.size() > 1 && normalized.back() == '/')
    normalized.pop_back();
  loc.alias = normalized;
}

static void applyAutoindex(Location &loc, Directive const &d) {
  std::string const &autoindex = d.args.at(0);

  if (autoindex == "on")
    loc.autoindex = true;
  else if (autoindex == "off")
    loc.autoindex = false;
  else
    throw std::runtime_error(utils::errorMsg("Unexpected value '" + autoindex + "' in 'autoindex' directive", d.line));
}

static void applyIndex(Location &loc, Directive const &d) {
  std::string const &index = d.args.at(0);

  if (index.empty())
    throw std::runtime_error(utils::errorMsg("Unexpected value '" + index +  "' in 'index' directive", d.line));
  loc.index = index;
}

static void applyClientMaxBodySize(Location &loc, Directive const &d) {
  std::string const &clientMaxBodySize = d.args.at(0);

  if (clientMaxBodySize.empty())
    throw std::runtime_error(utils::errorMsg("Unexpected value '" + clientMaxBodySize + "' in 'client_max_body_size' directive", d.line));
  
  char unit = clientMaxBodySize.back();
  size_t max = std::numeric_limits<size_t>::max();
  size_t scale = 1;
  std::string value = clientMaxBodySize;
  if (unit == 'k' || unit == 'K') {
    scale = 1024;
    max = std::numeric_limits<size_t>::max() / 1024;
    value = clientMaxBodySize.substr(0, clientMaxBodySize.size() - 1);
  } else if (unit == 'm' || unit == 'M') {
    max = std::numeric_limits<size_t>::max() / (1024 * 1024);
    scale = 1024 * 1024;
    value = clientMaxBodySize.substr(0, clientMaxBodySize.size() - 1);
  }

  if (!utils::isNumber(value) || value.empty()) 
    throw std::runtime_error(utils::errorMsg("Unexpected value '" + clientMaxBodySize + "' in 'client_max_body_size' directive", d.line));
    
  errno = 0;
  unsigned long s = std::strtoul(value.c_str(), NULL, 10);
  if (errno == ERANGE || s > max)
    throw std::runtime_error(utils::errorMsg("Unexpected value '" + clientMaxBodySize + "' in 'client_max_body_size' directive", d.line));
  
  loc.client_max_body_size = s * scale;
}

void applyDirective(Location &loc, Directive const &d) {
  if (d.name == "alias")
    applyAlias(loc, d);
  if (d.name == "autoindex")
    applyAutoindex(loc, d);
  if (d.name == "index")
    applyIndex(loc, d);
  if (d.name == "client_max_body_size")
    applyClientMaxBodySize(loc, d);
}

Location buildLocation(const Block &location, Location server_default_location) {
  Location default_location = server_default_location;
  applyLocationPath(default_location, location);
  for (size_t i = 0; i < location.directives.size(); ++i) {
    const Directive &d = location.directives[i];
    applyDirective(default_location, d);
  }
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
  std::vector<std::string> server_names;
  ListenerKey defaultKey;
  defaultKey.first = "0.0.0.0";
  defaultKey.second = 8000;
  for (size_t i = 0; i < server.directives.size(); ++i) {
    const Directive &d = server.directives[i];

    if (d.name == "listen")
      listens.push_back(makeListen(d));
    else if (d.name == "server_name")
      server_names = d.args;
    else
      applyDirective(server_default_location, d);
  }

  if (listens.empty()) {
    Server s;
    s.server_names = server_names;

    for (size_t j = 0; j < server.blocks.size(); ++j) {
      Location loc = buildLocation(server.blocks[j], server_default_location);
      s.locations.push_back(loc);
    }
    if (!hasRootLocation(s.locations))
      s.locations.push_back(server_default_location);
    listeners[defaultKey].push_back(s);
  }

  for (size_t i = 0; i < listens.size(); ++i) {
    Server s;
    s.server_names = server_names;

    for (size_t j = 0; j < server.blocks.size(); ++j) {
      Location loc = buildLocation(server.blocks[j], server_default_location);
      s.locations.push_back(loc);
    }
    if (!hasRootLocation(s.locations))
      s.locations.push_back(server_default_location);
    listeners[listens[i]].push_back(s);
  }
}

std::ostream &operator<<(std::ostream &out, Listeners const &l) {
  for (Listeners::const_iterator it = l.begin(); it != l.end(); ++it) {
    out << dbg::BOLD << dbg::BLUE << "{" << dbg::RESET << std::endl;
    out << "  " << dbg::BOLD << dbg::YELLOW << it->first.first << ":"
        << it->first.second << " {" << dbg::RESET << std::endl;
    for (size_t i = 0; i < it->second.size(); ++i)
      out << it->second[i];
    out << dbg::BOLD << dbg::YELLOW << "}" << dbg::RESET << std::endl;
  }
  return out;
}

std::ostream &operator<<(std::ostream &out, Server const &s) {
  out << dbg::BOLD << dbg::BLUE << "  server {" << dbg::RESET << std::endl;
  out << dbg::CYAN << "    server_names:" << dbg::RESET;
  for (size_t i = 0; i < s.server_names.size(); ++i) {
    out << " " << s.server_names[i];
  }
  out << std::endl;
  for (size_t i = 0; i < s.locations.size(); ++i) {
    out << s.locations[i];
  }
  out << dbg::BOLD << dbg::BLUE << "  }" << dbg::RESET << std::endl;
  return out;
}

std::ostream &operator<<(std::ostream &out, Location const &l) {
  out << dbg::BOLD << dbg::BLUE << "    location {" << dbg::RESET << std::endl;

  out << "      " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "path" << dbg::RESET << ": " << l.path << std::endl;

  out << "      " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "alias" << dbg::RESET << ": " << l.alias << std::endl;

  out << "      " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "autoindex" << dbg::RESET << ": "
      << (l.autoindex ? dbg::GREEN : dbg::RED) << (l.autoindex ? "on" : "off")
      << dbg::RESET << std::endl;

  out << "      " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "client_max_body_size" << dbg::RESET << ": " << dbg::YELLOW
      << l.client_max_body_size << dbg::RESET << std::endl;

  out << "      " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "index" << dbg::RESET << ": " << l.index << std::endl;

  out << "      " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "upload" << dbg::RESET << ": " << l.upload << std::endl;

  out << "      " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "limit_except" << dbg::RESET << ":";
  for (std::set<std::string>::iterator it = l.limit_except.begin();
       it != l.limit_except.end(); ++it) {
    out << " " << *it;
  }
  out << std::endl;

  out << "      " << dbg::CYAN << "error_pages" << dbg::RESET << " {";
  if (l.error_pages.size() > 0)
    out << std::endl;
  for (std::map<int, std::string>::const_iterator it = l.error_pages.begin();
       it != l.error_pages.end(); ++it) {
    out << "          " << dbg::YELLOW << it->first << dbg::RESET << " "
        << it->second << std::endl;
  }
  if (l.error_pages.size() > 0)
    out << "    ";
  out << "  }" << std::endl;

  out << "      " << dbg::CYAN << "returns" << dbg::RESET << " {";
  if (l.returns.size() > 0)
    out << std::endl;
  for (std::map<int, std::string>::const_iterator it = l.returns.begin();
       it != l.returns.end(); ++it) {
    out << "          " << dbg::YELLOW << it->first << dbg::RESET << " "
        << it->second << std::endl;
  }
  if (l.returns.size() > 0)
    out << "    ";
  out << "  }" << std::endl;

  out << dbg::BOLD << dbg::BLUE << "    }" << dbg::RESET << std::endl;

  return out;
}

Listeners buildConfig(const std::string &path) {
  Listeners listeners;
  Lexer lexer(path);
  Parser parser(lexer);
  Block http = parser.parse();
  validator::validate(http);

  Location http_default_location;
  std::map<int, std::string> returns;
  std::set<std::string> limit_except;
  std::map<int, std::string> error_pages;
  http_default_location.path = "/";
  http_default_location.alias = "/var/www";
  http_default_location.index = "index.html";
  http_default_location.returns = returns;
  http_default_location.client_max_body_size = 1024 * 1024;
  http_default_location.error_pages = error_pages;
  http_default_location.autoindex = false;
  http_default_location.upload = "";
  http_default_location.limit_except = limit_except;

  for (size_t i = 0; i < http.directives.size(); ++i)
    applyDirective(http_default_location, http.directives[i]);
  for (size_t i = 0; i < http.blocks.size(); ++i)
    buildServers(listeners, http.blocks[i], http_default_location);

  std::cout << listeners;
  return listeners;
}
}; // namespace config

// for (size_t i = 1; i < d.args.size() - 1; ++i) {
//     int status = std::atoi(d.args[i].c_str());
//     if (d.args[i].length() != 3 || status < 300 || status > 599)
//       throw std::runtime_error(
//           utils::errorMsg("Invalid value '" + d.args[i] + "'", d.line));
//   }
