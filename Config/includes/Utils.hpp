#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

namespace utils
{
    std::string errorMsg(std::string const& message, std::size_t line);
    std::string toString(std::size_t val);
    bool isNumber(const std::string& s);
    std::string validatePath(std::string const& path, const std::string& name, std::size_t line);
} // namespace utils

#endif // !UTILS_HPP
