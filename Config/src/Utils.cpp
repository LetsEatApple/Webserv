#include "Utils.hpp"
#include <sstream>

namespace utils {
std::string errorMsg(std::string const &message, const size_t line) {
  std::stringstream err;
  err << message << " at line " << line;
  return err.str();
}

std::string toString(const size_t val) {
  std::stringstream str;
  str << val;
  return str.str();
}

bool isNumber(const std::string &s) {
  if (s.empty())
    return false;
  for (size_t i = 0; i < s.size(); ++i)
    if (!std::isdigit(s[i]))
      return false;
  return true;
}

  std::string validatePath(std::string const &path, std::string const& name, const std::size_t line) {
  if (path.empty() || path[0] != '/')
    throw std::runtime_error(
        utils::errorMsg("Unexpected value '" + path + "' in '" + name + "' directive", line));

  std::string normalized = path;
  if (normalized.size() > 1 && normalized[normalized.size() - 1] == '/')
    normalized = normalized.substr(0, normalized.size() - 1);

  return normalized;
}
} // namespace utils
