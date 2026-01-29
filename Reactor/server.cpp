/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:07:37 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/27 22:03:16 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

/* Server::Server(int nports, char** ports) : nports(nports){

	s_list.resize(nports);
	for (int i = 0; i < nports; i++){
		s_list[i].port = atoi(ports[i+1]);
		std::memset(&s_list[i].s_addr, 0, sizeof(s_list[i].s_addr));
		s_list[i].s_addr.sin_port = htons(s_list[i].port); 
		//s_list[i].s_addr.sin_port = htons(80); //for error testing (bind)
		s_list[i].s_addr.sin_family = AF_INET;
		s_list[i].s_addr.sin_addr.s_addr = INADDR_ANY;
		s_list[i].sock = -1;
	}
} */

/* Server::Server(config::Listeners &listeners) : listeners(listeners){

	nports = listeners.size();
	s_list.resize(nports);
	config::Listeners::iterator it;
	int i = 0;

	for (it = listeners.begin(); it != listeners.end(); ++it){

		s_list[i].port = it->first.second;
		std::memset(&s_list[i].s_addr, 0, sizeof(s_list[i].s_addr));
		s_list[i].s_addr.sin_port = htons(s_list[i].port); 
		s_list[i].s_addr.sin_family = AF_INET;
		s_list[i].s_addr.sin_addr.s_addr = INADDR_ANY;
		s_list[i].sock = -1;
		i++;
	}
	
} */

std::string str(uint16_t port){

	std::stringstream ss;
	ss << port;
	std::string s_port = ss.str();
	return s_port;
}

void freeaddr(server_list &s_list, int n){

	for (int i = 0; i < n; i++)
		freeaddrinfo(s_list[i].n_server);
}

Server::Server(config::Listeners &listeners) : listeners(listeners){

	nports = listeners.size();
	s_list.resize(nports);
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC; // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE; // if host is NULL → bind to 0.0.0.0 / ::if host is set → bind to that exact address
	
	config::Listeners::iterator it;
	int i = 0;
	for (it = listeners.begin(); it != listeners.end(); ++it){

		s_list[i].port = it->first.second;
		s_list[i].s_port = str(it->first.second);
		s_list[i].host = it->first.first;
		if (getaddrinfo(s_list[i].host.c_str(), s_list[i].s_port.c_str(), &hints, &s_list[i].n_server) != 0){
			freeaddr(s_list, i);
			throw InvalidAddrException();
		}
		s_list[i].sock = -1;
		i++;
	}
	
}

int Server::createServer(){
	
	for (int i = 0; i < static_cast<int>(s_list.size()); i++){
		// socket
		s_list[i].sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (s_list[i].sock < 0) // close all sockets and return? set sock -1 before starting loop? or in main on error check
			return error(1);
		
		// bind
		int bind_val = bind(s_list[i].sock, (struct sockaddr *)s_list[i].n_server->ai_addr, s_list[i].n_server->ai_addrlen);
		if(bind_val < 0) // open fds
			return error(2);

		// listen
		int listen_val = listen(s_list[i].sock, 1);
		if(listen_val < 0)
			return error(3);
	}
	return true;
}

Server::~Server(){

	for (int i = 0; i < nports; i++){
		if (s_list[i].sock != -1)
			close (s_list[i].sock);
		freeaddrinfo(s_list[i].n_server);
	}
	// close every lasting fd in _handler instead?
}

const char* Server::InvalidAddrException::what() const throw(){
	return "Error: getaddrinfo: Invalid address";
}