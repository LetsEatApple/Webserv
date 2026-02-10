

#include <sys/types.h>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <arpa/inet.h>
#include <sstream>

/* int main() {

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
} */

/* int main(){
	
	/* std::string	hex = "2F";
	if (!std::isxdigit(hex[0]) || !std::isxdigit(hex[1]))
		return 0;
	int dec = std::strtoul(hex.c_str(), NULL, 16);
	if (dec <= 0 || dec > 255) // should %00 (hex) = \0 (char) = 0 (dec)  be valid?
		return 0;
	std::cout << "char: " << static_cast<char> (dec) << std::endl;


	std::string s = "Hello, world!";
	std::string prefix = "Hello";
	if (s.rfind(prefix, 0) == 0)
	    std::cout << "s starts with '" << prefix << "'\n";

	std::string pf = "world";
	if (s.rfind(pf, 0) == 0)
	    std::cout << "s starts with '" << pf << "'\n";
	else
		std::cout << "does not start with " << pf << std::endl;

} */


int main(){
	
	std::string input = " mid/content=5/../6";
	std::string output = "";

	while(!input.empty()){

		// A: input buffer begins with a prefix of "../" or "./" remove prefix
		if (input.rfind("../", 0) == 0 || input.rfind("./", 0) == 0)
			input.erase(0, input.find('/') +1);

		// B: begins with "/./" or "/." replace with "/"
		else if(input.rfind("/./", 0) == 0 || (input.rfind("/.", 0) == 0 && input.rfind("/..", 0) != 0)){
			if (input.rfind("/./", 0) == 0)
				input.replace(0, 3, "/");
			else
				input.replace(0, 2, "/");
		}

		// C: begins with "/../" or "/.." replace with "/" and remove last segment and its preceding "/" from output
		else if (input.rfind("/../", 0) == 0 || input.rfind("/..", 0) == 0){
			if (input.rfind("/../", 0) == 0)
				input.replace(0, 4, "/");
			else
				input.replace(0, 3, "/");
			size_t found = output.find_last_of('/');
			if (found != std::string::npos)
				output.erase(found);
		}

		// D: if consits only of "." or ".." remove
		else if (input.compare("..") == 0 || input.compare(".") == 0)
			input.clear();

		// E: move first path segment to end of output buffer, including initial "/" and any subsequent chars up to, but not incl next "/" or the end of input
		else{ 
			size_t n = input.find('/', 1);
			if (n != std::string::npos){
				output.append(input, 0, n);
				input.erase(0, n);
			}
			else{
				output.append(input);
				input.clear();
			}
		}
	}
	std::cout << output << std::endl;
}