/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:07:37 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/14 18:43:28 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

void freeaddr(server_list &_servers, int n){

	for (int i = 0; i < n; i++)
		freeaddrinfo(_servers[i].n_server);
}

Server::Server(config::Listeners &listeners) /*: listeners(listeners)*/{

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC; // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE; // if host is NULL → bind to 0.0.0.0 / ::if host is set → bind to that exact address
	_servers.resize(listeners.size());

	config::Listeners::iterator it;
	int i = 0;
	for (it = listeners.begin(); it != listeners.end(); ++it){

		std::stringstream ss;
		_servers[i].portNum = it->first.second;
		_servers[i].portStr = str(it->first.second);
		_servers[i].host = it->first.first;
		_servers[i].session.vhosts = it->second;
		ss << _servers[i].host << "\033[95m:\033[0;36m" << _servers[i].portStr;
		_servers[i].session.listener.portPair = ss.str();
		if (getaddrinfo(_servers[i].host.c_str(), _servers[i].portStr.c_str(), &hints, &_servers[i].n_server) != 0){
			freeaddr(_servers, i);
			throw InvalidAddrException();
		}
		_servers[i].sock = -1;
		i++;
	}

}

int	Server::error_close(std::string err, int index){
	for (int i = 0; i < index; i++)
		close(_servers[i].sock);
	return error(err, 1);
}

int Server::createServer(){

	for (int i = 0; i < static_cast<int>(_servers.size()); i++){

		// socket and pair
		_servers[i].sock = socket(_servers[i].n_server->ai_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (_servers[i].sock < 0) // close all sockets and return? set sock -1 before starting loop? or in main on error check
			return error_close("creaing socket failed", i);
		_servers[i].session.listener.socket = _servers[i].sock;
		_sessions.push_back(_servers[i].session);
		int opt = 1; // allow immediate restart
		setsockopt(_servers[i].sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		// bind
		int bind_val = bind(_servers[i].sock, (struct sockaddr *)_servers[i].n_server->ai_addr, _servers[i].n_server->ai_addrlen);
		if(bind_val < 0)
			return error_close("binding socket failed", i);

		// listen
		int listen_val = listen(_servers[i].sock, 1);
		if(listen_val < 0)
			return error_close("listening to socket failed", i);
		std::stringstream ss;
		ss << _servers[i].host << "\033[95m:\033[0;36m" << _servers[i].portStr << " (fd=" << _servers[i].sock << ")";
		logInfo("Listening on ", ss.str());
	}

	return true;
}

const activeSessions	&Server::get_sessions(){
	return _sessions;
}

Server::~Server() {
	for (size_t i = 0; i < _servers.size(); i++)
		freeaddrinfo(_servers[i].n_server);
}

const char* Server::InvalidAddrException::what() const throw(){
	return "getaddrinfo: Invalid address";
}