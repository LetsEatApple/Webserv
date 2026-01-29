

#include <sys/types.h>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <arpa/inet.h>
#include <sstream>

int main() {

	std::string host = "127.0.0.1";
	std::string port = "8080";

	uint16_t p = 8080;
	std::string convert;

	std::stringstream ss;
	ss << p;
	convert = ss.str();

	std::cout << "c = " << convert << std::endl;

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;     // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE; 
	struct addrinfo* res;

	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0)
		std::cout << "Error" << std::endl;
	
	 char ipstr[INET6_ADDRSTRLEN]; // buffer big enough for IPv6

    for (struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
        void* addr;
        std::string ipver;

        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else if (p->ai_family == AF_INET6) { // IPv6
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        } else {
            continue; // ignore other families
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        std::cout << ipver << ": " << ipstr << std::endl;
}
}