/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:07:37 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/31 16:38:02 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

/* Server::Server(int nports, char** ports) : nports(nports){

	_slist.resize(nports);
	for (int i = 0; i < nports; i++){
		_slist[i].port = atoi(ports[i+1]);
		std::memset(&_slist[i].s_addr, 0, sizeof(_slist[i].s_addr));
		_slist[i].s_addr.sin_port = htons(_slist[i].port); 
		//_slist[i].s_addr.sin_port = htons(80); //for error testing (bind)
		_slist[i].s_addr.sin_family = AF_INET;
		_slist[i].s_addr.sin_addr.s_addr = INADDR_ANY;
		_slist[i].sock = -1;
	}
} */

/* Server::Server(config::Listeners &listeners) : listeners(listeners){

	nports = listeners.size();
	_slist.resize(nports);
	config::Listeners::iterator it;
	int i = 0;

	for (it = listeners.begin(); it != listeners.end(); ++it){

		_slist[i].port = it->first.second;
		std::memset(&_slist[i].s_addr, 0, sizeof(_slist[i].s_addr));
		_slist[i].s_addr.sin_port = htons(_slist[i].port);
		_slist[i].s_addr.sin_family = AF_INET;
		_slist[i].s_addr.sin_addr.s_addr = INADDR_ANY;
		_slist[i].sock = -1;
		i++;
	}

} */

std::string str(uint16_t port){

	std::stringstream ss;
	ss << port;
	std::string s_port = ss.str();
	return s_port;
}

void freeaddr(server_list &_slist, int n){

	for (int i = 0; i < n; i++)
		freeaddrinfo(_slist[i].n_server);
}

Server::Server(config::Listeners &listeners) /*: listeners(listeners)*/{

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC; // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE; // if host is NULL → bind to 0.0.0.0 / ::if host is set → bind to that exact address

	config::Listeners::iterator it;
	int i = 0;
	for (it = listeners.begin(); it != listeners.end(); ++it){

		_slist[i].portNum = it->first.second;
		_slist[i].portStr = str(it->first.second);
		_slist[i].host = it->first.first;
		_slist[i].ls.second = it->second;
		if (getaddrinfo(_slist[i].host.c_str(), _slist[i].portStr.c_str(), &hints, &_slist[i].n_server) != 0){
			freeaddr(_slist, i);
			throw InvalidAddrException();
		}
		_slist[i].sock = -1;
		i++;
	}

}

int	Server::error_close(int err, int index){
	for (int i = 0; i < index; i++)
		close(_slist[i].sock);
	return error(err);
}

int Server::createServer(){

	for (int i = 0; i < static_cast<int>(_slist.size()); i++){

		// socket and pair
		_slist[i].sock = socket(_slist[i].n_server->ai_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (_slist[i].sock < 0) // close all sockets and return? set sock -1 before starting loop? or in main on error check
			return error_close(1, i); //close fd
		_slist[i].ls.first = _slist[i].sock;
		_listeners.push_back(_slist[i].ls);
		int opt = 1; // allow immediate restart
		setsockopt(_slist[i].sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		// bind
		int bind_val = bind(_slist[i].sock, (struct sockaddr *)_slist[i].n_server->ai_addr, _slist[i].n_server->ai_addrlen);
		if(bind_val < 0) // open fds
			return error_close(2, i); //close fd

		// listen
		int listen_val = listen(_slist[i].sock, 1);
		if(listen_val < 0)
			return error_close(3, i); //close fd
	}

	return true;
}

const serverListeners	&Server::get_listeners(){
	return _listeners;
}

Server::~Server() {}

const char* Server::InvalidAddrException::what() const throw(){
	return "Error: getaddrinfo: Invalid address";
}