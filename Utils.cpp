#include "Utils.hpp"
#include <sstream>

namespace utils {
const std::string errorMsg(std::string const &message, size_t line) {
  std::stringstream err;
  err << message << " at line " << line;
  return err.str();
}

bool isNumber(const std::string &s) {
  if (s.empty())
    return false;
  for (size_t i = 0; i < s.size(); ++i)
    if (!std::isdigit(s[i]))
      return false;
  return true;
}

} // namespace utils
