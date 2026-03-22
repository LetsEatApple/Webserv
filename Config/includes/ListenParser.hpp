#ifndef LISTENPARSER_HPP
#define LISTENPARSER_HPP
#include "AST.hpp"
#include <stdint.h>

namespace config
{
	std::string getAddress(std::string& address, std::string& port,
						   const Directive& d, bool isV6);
	uint16_t getPort(const std::string& portStr, const Directive& d);
	std::pair<std::string, uint16_t> makeListen(const Directive& d);
} // namespace config

#endif // !LISTENPARSER_HPP
