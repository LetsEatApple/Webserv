/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:07:37 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/24 17:33:49 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

Server::Server(int nports, char** ports, RequestHandler* requestHandler) : _requestHandler(requestHandler), _s_sock(-1), _c_sock(-1), _nports(nports) {

	_s_list.resize(nports);
	for (int i = 0; i < nports; i++){
		_s_list[i].port = atoi(ports[i+1]);
		std::memset(&_s_list[i].s_addr, 0, sizeof(_s_list[i].s_addr));
		_s_list[i].s_addr.sin_port = htons(_s_list[i].port);
		//s_list[i].s_addr.sin_port = htons(80); //for error testing (bind)
		_s_list[i].s_addr.sin_family = AF_INET;
		_s_list[i].s_addr.sin_addr.s_addr = INADDR_ANY;
		_s_list[i].sock = -1;
	}
}

RequestHandler* Server::getRequestHandler() const{
	return _requestHandler;
}

int Server::getNports() const{
	return _nports;
}

std::vector<struct server_list>& Server::getServerList(){
	return _s_list;
}

int Server::createServer(){

	for (int i = 0; i < static_cast<int>(_s_list.size()); i++){
		// socket
		_s_list[i].sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (_s_list[i].sock < 0) // close all sockets and return? set sock -1 before starting loop? or in main on error check
			return error(1);

		// bind
		int bind_val = bind(_s_list[i].sock, (struct sockaddr *)&_s_list[i].s_addr, sizeof(_s_list[i].s_addr));
		if(bind_val < 0) // open fds
			return error(2);

		// listen
		int listen_val = listen(_s_list[i].sock, 1);
		if(listen_val < 0)
			return error(3);
	}
	return true;
}

Server::~Server(){

	for (int i = 0; i < _nports; i++){
		if (_s_list[i].sock != -1)
			close (_s_list[i].sock);
	}
}