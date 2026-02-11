#include "Logger.hpp"

int	error(std::string err, int errno_on){

	std::cout << "\033[0;31mError: ";
	std::cout << err;
	if (errno_on)
		std::cout << ": " << strerror(errno);
	std::cout << "\033[0m" << std::endl;

	if (!errno_on)
		return 1;
	return 0;
}

void logDebug(std::string err, int errno_on){

	(void) err;
	(void) errno_on;
	#ifdef DEBUG
	std::cout << "\033[0;31m[Error] ";
	std::cout << err;
	if (errno_on)
		std::cout << ": " << strerror(errno);
	std::cout << "\033[0m" << std::endl;
	#endif
	
}

void logInfo(std::string info, std::string arg){

	(void) info;
	#ifdef DEBUG
	std::cout << "[\033[90mInfo\033[0m] " << info
	<< "\033[0;36m" << arg << "\033[0m";
	std::cout << std::endl;

	#endif

}