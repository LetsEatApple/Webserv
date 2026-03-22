#ifndef CONFIGPRINTER_HPP
#define CONFIGPRINTER_HPP


#include "Config.hpp"
#include <ostream>

namespace config
{
	std::ostream& operator<<(std::ostream& out, Location const& l);
	std::ostream& operator<<(std::ostream& out, Server const& s);
	std::ostream& operator<<(std::ostream& out, Listeners const& l);
} // namespace config
#endif // !CONFIGPRINTER_HPP
