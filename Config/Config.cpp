#include "Config.hpp"
#include "AST.hpp"
#include "Debug.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Utils.hpp"
#include "Validator.hpp"
#include <algorithm>
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

static std::string getAddress(std::string &address, std::string &port,
                              Directive const &d, bool isV6) {
  if (address == "*")
    address = "0.0.0.0";
  if (address == "localhost")
    address = "127.0.0.1";

  struct addrinfo hints;
  struct addrinfo *res = NULL;
  
  std::memset(&hints, 0, sizeof(hints));
  
  hints.ai_family = isV6 ? AF_INET6 : AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  
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

static std::pair<std::string, uint16_t> makeListen(Directive const &d) {
  std::string arg = d.args[0];
  std::string address = "0.0.0.0";
  uint16_t port = 8000;

  if (!arg.empty() && arg[0] == '[') {
    size_t close = arg.find(']');
    if (close == std::string::npos || close == 1)
      throw std::runtime_error(
          utils::errorMsg("Invalid IPv6 address format in '" + arg +
                              "' of the 'listen' directive",
                          d.line));

    address = arg.substr(1, close - 1);
    if (address == "*")
      throw std::runtime_error(
          utils::errorMsg("Invalid IPv6 address format in '" + arg +
                              "' of the 'listen' directive",
                          d.line));

    if (close + 1 == arg.size()) {
      std::string portStr = utils::toString(port);
      address = getAddress(address, portStr, d, true);
    } else {
      if (arg[close + 1] != ':')
        throw std::runtime_error(
            utils::errorMsg("Invalid IPv6 address format in '" + arg +
                                "' of the 'listen' directive",
                            d.line));

      std::string portStr = arg.substr(close + 2);
      port = getPort(portStr, d);
      address = getAddress(address, portStr, d, true);
    }
  } else if (arg.find(':') != std::string::npos) {
    size_t colon = arg.find(':');

    if (arg.find(':', colon + 1) != std::string::npos)
      throw std::runtime_error(
          utils::errorMsg("IPv6 address must be enclosed in [] in '" + arg +
                              "' of the 'listen' directive",
                          d.line));

    address = arg.substr(0, colon);
    std::string portStr = arg.substr(colon + 1);
    port = getPort(portStr, d);
    address = getAddress(address, portStr, d, false);
  }

  else if (utils::isNumber(arg))
    port = getPort(arg, d);
  else {
    std::string partStr = utils::toString(port);
    address = getAddress(arg, partStr, d, false);
  }

  return std::make_pair(address, port);
}

static std::string validatePath(std::string const &path, std::string const name, int line) {
  if (path.empty() || path[0] != '/')
    throw std::runtime_error(
        utils::errorMsg("Unexpected value '" + path + "' in '" + name + "' directive", line));

  std::string normalized = path;
  if (normalized.size() > 1 && normalized[normalized.size() - 1] == '/')
    normalized = normalized.substr(0, normalized.size() - 1);

  return normalized;  
}

static void applyLocationPath(Location &loc, Block const &b) {
  std::string const &path = b.args.at(0);
  loc.path = validatePath(path, b.name, b.line);
}

static void applyAlias(Location &loc, Directive const &d) {
  std::string const &alias = d.args.at(0);
  loc.alias = validatePath(alias, d.name, d.line);
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

static void applyUpload(Location &loc, Directive const &d) {
  std::string const &upload = d.args.at(0);

  if (upload.empty()) {
    loc.upload = "";
    return;
  }
  loc.upload = validatePath(upload, d.name, d.line);
}

static void applyLimitExcept(Location &loc, Directive const &d) {
  std::set<std::string>  methods;
  for (size_t i = 0; i < d.args.size(); ++i) {
    std::string const &method = d.args[i];

    if (method == "GET" || method == "POST" || method == "DELETE")
      methods.insert(method);
    else
      throw std::runtime_error(utils::errorMsg("Unexpected value '" + method + "' in 'limit_except' directive", d.line));
  }
  loc.limit_except = methods;
}

static void applyClientMaxBodySize(Location &loc, Directive const &d) {
  std::string const &clientMaxBodySize = d.args.at(0);

  if (clientMaxBodySize.empty())
    throw std::runtime_error(utils::errorMsg("Unexpected value '" + clientMaxBodySize + "' in 'client_max_body_size' directive", d.line));
  
  char unit = clientMaxBodySize[clientMaxBodySize.size() - 1];
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

void applyErrorPages(Location &loc, Directive const &d) {
  std::map<int, std::string> error_page;
  
  std::string normalized = validatePath(d.args[d.args.size() - 1], d.name, d.line);
  for (size_t i = 0; i < d.args.size() - 1; ++i) {
    int status = std::atoi(d.args[i].c_str());
    if (d.args[i].length() != 3 || status < 300 || status > 599)
      throw std::runtime_error(
      utils::errorMsg("Unexpected value '" + d.args[i] + "' in 'error_page' directive", d.line));
    error_page.insert(std::pair<int, std::string>(status, normalized));
  }
  loc.error_page = error_page;
}

void applyReturn(Location &loc, Directive const &d) { 
  int status = std::atoi(d.args.at(0).c_str());
  std::string normalized = d.args[d.args.size() - 1];
  if (normalized.at(0) == '/')
    normalized = validatePath(d.args[d.args.size() - 1], d.name, d.line);
  if (d.args.at(0).length() != 3 || status < 100 || status > 599)
      throw std::runtime_error(
      utils::errorMsg("Unexpected value '" + d.args[0] + "' in '" + d.name + "' directive", d.line));
    loc.ret.insert(std::pair<int, std::string>(status, normalized));
}

void applyCGI(Location &loc, Directive const &d) {
  std::string extension = d.args.at(0);
  if (extension.at(0) != '.')
    throw std::runtime_error(utils::errorMsg("Unexpected value '" + d.args[0] + "' in '" + d.name + "' directive", d.line));
  std::string normalized = d.args.at(1);
  if (normalized.at(0) == '/')
    normalized = validatePath(d.args.at(1), d.name, d.line);
  loc.cgi.insert(std::pair<std::string, std::string>(extension, normalized));
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
  if (d.name == "upload")
    applyUpload(loc, d);
  if (d.name == "limit_except")
    applyLimitExcept(loc, d);
  if (d.name == "error_page")
    applyErrorPages(loc, d);
  if (d.name == "return")
    applyReturn(loc, d);
  if (d.name == "cgi")
    applyCGI(loc, d);
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

static bool sortLocation(const Location &a, const Location &b) {
    return a.path.size() > b.path.size();
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

  if (server_names.size() == 0)
    server_names.push_back("");

  if (listens.empty()) {
    Server s;
    s.server_names = server_names;

    for (size_t j = 0; j < server.blocks.size(); ++j) {
      Location loc = buildLocation(server.blocks[j], server_default_location);
      s.locations.push_back(loc);
    }
    if (!hasRootLocation(s.locations))
      s.locations.push_back(server_default_location);
    
    std::set<std::string> existing_names;
    for (size_t j = 0; j < listeners[defaultKey].size(); ++j) {
        const Server &existing_server = listeners[defaultKey][j];
        for (size_t l = 0; l < existing_server.server_names.size(); ++l) {
            existing_names.insert(existing_server.server_names[l]);
        }
    }
    for (size_t k = 0; k < s.server_names.size(); ++k) {
        if (existing_names.find(s.server_names[k]) != existing_names.end()) {
            throw std::runtime_error(
                "conflicting server name '" + s.server_names[k] + "' on 127.0.0.1:8000");
        }
    }
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
    std::sort(s.locations.begin(), s.locations.end(), sortLocation);

    std::set<std::string> existing_names;
    for (size_t j = 0; j < listeners[listens[i]].size(); ++j) {
        const Server &existing_server = listeners[listens[i]][j];
        for (size_t l = 0; l < existing_server.server_names.size(); ++l) {
            existing_names.insert(existing_server.server_names[l]);
        }
    }
    for (size_t k = 0; k < s.server_names.size(); ++k) {
        if (existing_names.find(s.server_names[k]) != existing_names.end()) {
            throw std::runtime_error("conflicting server name '" + s.server_names[k] + "' on " + listens[i].first + ":" + utils::toString(listens[i].second));
        }
    }

    listeners[listens[i]].push_back(s);
  }
}

std::ostream &operator<<(std::ostream &out, Listeners const &l) {
  for (Listeners::const_iterator it = l.begin(); it != l.end(); ++it) {
    out << dbg::BOLD << dbg::BLUE << "[" << dbg::RESET << std::endl;
    out << "  " << dbg::BOLD << dbg::YELLOW << it->first.first << ":"
        << it->first.second << " {" << dbg::RESET << std::endl;
    for (size_t i = 0; i < it->second.size(); ++i)
      out << it->second[i];
    out << dbg::BOLD << dbg::YELLOW << "  }" << dbg::RESET << std::endl;
    out << dbg::BOLD << dbg::BLUE << "]" << dbg::RESET << std::endl;
  }
  return out;
}

std::ostream &operator<<(std::ostream &out, Server const &s) {
  out << dbg::BOLD << dbg::MAGENTA << "    server {" << dbg::RESET << std::endl;
  out << dbg::BOLD << dbg::BLUE << "      server_name:" << dbg::RESET;
  for (size_t i = 0; i < s.server_names.size(); ++i) {
    out << " " << s.server_names[i];
  }
  out << std::endl;
  for (size_t i = 0; i < s.locations.size(); ++i) {
    out << s.locations[i];
  }
  out << dbg::BOLD << dbg::MAGENTA << "    }" << dbg::RESET << std::endl;
  return out;
}

std::ostream &operator<<(std::ostream &out, Location const &l) {
  out << dbg::BOLD << dbg::BLUE << "      location {" << dbg::RESET << std::endl;

  out << "        " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "path" << dbg::RESET << ": " << l.path << std::endl;

  out << "        " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "alias" << dbg::RESET << ": " << l.alias << std::endl;

  out << "        " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "autoindex" << dbg::RESET << ": "
      << (l.autoindex ? dbg::GREEN : dbg::RED) << (l.autoindex ? "on" : "off")
      << dbg::RESET << std::endl;

  out << "        " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "client_max_body_size" << dbg::RESET << ": " << dbg::YELLOW
      << l.client_max_body_size << dbg::RESET << std::endl;

  out << "        " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "index" << dbg::RESET << ": " << l.index << std::endl;

  if (l.upload.empty()) {
    out << "        " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "upload" << dbg::RESET << ": " << dbg::RED << "off" << dbg::RESET << std::endl;
  } else {
    out << "        " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "upload" << dbg::RESET << ": " << l.upload << std::endl;
  }

  out << "        " << dbg::CYAN << std::left << std::setw(dbg::KEY_WIDTH)
      << "limit_except" << dbg::RESET << ":";
  for (std::set<std::string>::iterator it = l.limit_except.begin();
       it != l.limit_except.end(); ++it) {
    out << " " << *it;
  }
  out << std::endl;

  out << "        " << dbg::CYAN << "error_page" << dbg::RESET << " {";
  if (l.error_page.size() > 0)
    out << std::endl;
  for (std::map<int, std::string>::const_iterator it = l.error_page.begin();
       it != l.error_page.end(); ++it) {
    out << "            " << dbg::YELLOW << it->first << dbg::RESET << " "
        << it->second << std::endl;
  }
  if (l.error_page.size() > 0)
    out << "      ";
  out << "    }" << std::endl;

  out << "        " << dbg::CYAN << "return" << dbg::RESET << " {";
  if (l.ret.size() > 0)
    out << std::endl;
  for (std::map<int, std::string>::const_iterator it = l.ret.begin();
       it != l.ret.end(); ++it) {
    out << "            " << dbg::YELLOW << it->first << dbg::RESET << " "
        << it->second << std::endl;
  }
  if (l.ret.size() > 0)
    out << "      ";
  out << "    }" << std::endl;
  
  out << "        " << dbg::GRAY << "cgi" << " {";
  if (l.cgi.size() > 0)
    out << std::endl;
  for (std::map<std::string, std::string>::const_iterator it = l.cgi.begin();
       it != l.cgi.end(); ++it) {
    out << "            " << it->first << " "
        << it->second << std::endl;
  }
  if (l.cgi.size() > 0)
    out << "      ";
  out << "    }" << std::endl;

  out << "      }" << dbg::RESET << std::endl;

  return out;
}

Listeners buildConfig(const std::string &path) {
  Listeners listeners;
  Lexer lexer(path);
  Parser parser(lexer);
  Block http = parser.parse();
  validator::validate(http);

  Location http_default_location;
  std::map<int, std::string> ret;
  std::map<std::string, std::string> cgi;
  std::set<std::string> limit_except;
  limit_except.insert("GET");
  limit_except.insert("POST");
  limit_except.insert("DELETE");
  std::map<int, std::string> error_page;
  http_default_location.path = "/";
  http_default_location.alias = "/var/www";
  http_default_location.index = "index.html";
  http_default_location.ret = ret;
  http_default_location.client_max_body_size = 1024 * 1024;
  http_default_location.error_page = error_page;
  http_default_location.autoindex = false;
  http_default_location.upload = "";
  http_default_location.limit_except = limit_except;
  http_default_location.cgi = cgi;

  for (size_t i = 0; i < http.directives.size(); ++i)
    applyDirective(http_default_location, http.directives[i]);
  for (size_t i = 0; i < http.blocks.size(); ++i)
    buildServers(listeners, http.blocks[i], http_default_location);

  std::cout << listeners;
  return listeners;
}
}; // namespace config
