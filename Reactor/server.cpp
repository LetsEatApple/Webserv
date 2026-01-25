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

Server::Server(int nports, char** ports) : nports(nports) {

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
}

int Server::createServer(){
	
	for (int i = 0; i < static_cast<int>(s_list.size()); i++){
		// socket
		s_list[i].sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (s_list[i].sock < 0) // close all sockets and return? set sock -1 before starting loop? or in main on error check
			return error(1);
		
		// bind
		int bind_val = bind(s_list[i].sock, (struct sockaddr *)&s_list[i].s_addr, sizeof(s_list[i].s_addr));
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
	}
	// close every lasting fd in _handler instead?
}