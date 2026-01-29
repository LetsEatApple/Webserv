#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

namespace utils {
const std::string errorMsg(std::string const &message, size_t line);
const std::string toString(size_t val);
bool isNumber(const std::string &s);
} // namespace utils

#endif // !UTILS_HPP
