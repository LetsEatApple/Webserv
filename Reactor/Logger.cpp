#include "Logger.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <netinet/in.h>

int	error(const std::string& err, const int errno_on){

	std::cout << "\033[0;31mError: ";
	std::cout << err;
	if (errno_on)
		std::cout << ": " << strerror(errno);
	std::cout << "\033[0m" << std::endl;

	if (!errno_on)
		return 1;
	return 0;
}

std::string str(const int port){

	std::stringstream ss;
	ss << port;
	std::string s_port = ss.str();
	return s_port;
}

void logDebug(const std::string& err, const int errno_on){

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

void logInfo(const std::string& info, const std::string& arg){

	(void) info;
	(void) arg;
	#ifdef DEBUG
	std::cout << "[\033[90mInfo\033[0m] " << info
	<< "\033[0;36m" << arg << "\033[0m";
	std::cout << std::endl;

	#endif

}

std::string IP_port(const sockaddr_in &c_addr){

	const unsigned long ip_raw = ntohl(c_addr.sin_addr.s_addr);
	const unsigned char b1 = (ip_raw >> 24) & 0xFF;
	const unsigned char b2 = (ip_raw >> 16) & 0xFF;
	const unsigned char b3 = (ip_raw >> 8)  & 0xFF;
	const unsigned char b4 = ip_raw & 0xFF;

	const int port = ntohs(c_addr.sin_port);

	std::ostringstream oss;
	oss << static_cast<int>(b1) << "."
		<< static_cast<int>(b2) << "."
		<< static_cast<int>(b3) << "."
		<< static_cast<int>(b4)
		<< "\033[95m:\033[0;36m" << port;

	return oss.str();
}

void logConnection(const Endpoint& listener, const Endpoint& client){
	(void) listener;
	(void) client;
	#ifdef DEBUG

	std::cout << "[\033[90mInfo\033[0m] " << "+ Accepted new client @"
	<< "\033[0;36m" << client.portPair << " (fd=" << client.socket << ") \033[0m->\033[0;36m "
	<< listener.portPair << " (fd=" << listener.socket << ")\033[0m";
	std::cout << std::endl;

	#endif
}

void logDisconnect(const Endpoint& listener, const Endpoint& client){

	(void) listener;
	(void) client;
	#ifdef DEBUG

	std::cout << "[\033[90mInfo\033[0m] " << "- Client @"
	<< "\033[0;36m" << client.portPair << " (fd=" << client.socket << ") \033[0m->\033[0;36m "
	<< listener.portPair << " (fd=" << listener.socket << ")\033[0m disconnected" << std::endl;

	#endif
}

void logRequest(const Endpoint& listener, const Endpoint& client, const Request& req, const std::string& locationPath){

	(void) listener;
	(void) client;
	(void) req;
	(void) locationPath;

	#ifdef DEBUG

	std::cout << "[\033[90mInfo\033[0m] " << "Request received from Client @"
	<< "\033[0;36m" << client.portPair << " (fd=" << client.socket << ") \033[0m->\033[0;36m "
	<< listener.portPair << " (fd=" << listener.socket << ")\033[0m :" << std::endl;


	Headers headers = req.getHeaders();

	std::cout << "\n\t\033[95mMethod: \033[0;36m" << req.getMethod()
	<< "\n\t\033[95mRequest target: \033[0;36m" << req.getTarget()
	<< "\n\t\033[95mTarget path: \033[0;36m" << req.getPath()
	<< "\n\t\033[95mTarget query: \033[0;36m" << req.getQuery()
	<< "\n\t\033[95mHost: \033[0;36m" << headers.get("host")
	<< "\n\t\033[95mmatched Location (path): \033[0;36m" << locationPath << "\033[0;37m\n"
	<< std::endl;
	#endif
}