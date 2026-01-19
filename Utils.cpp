#include "Utils.hpp"
#include <sstream>

namespace Utils {
const std::string errorMsg(std::string const &message, size_t line) {
  std::stringstream err;
  err << message << " at line " << line;
  return err.str();
}
} // namespace Utils
