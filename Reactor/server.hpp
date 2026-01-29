/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:06:56 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/23 13:04:03 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.h"

struct server{
	int sock;
	int port;
	std::string s_port;
	std::string host; //addr
	struct sockaddr_in s_addr;
	struct addrinfo *n_server; //network server
};

typedef std::vector<struct server> server_list;

class Server{
private:
	Server();
public:
	//Server(int nports, char **ports);
	Server(config::Listeners &listeners);
	config::Listeners&	listeners;
	server_list			s_list;
	int					nports;
	int					createServer();

	class InvalidAddrException : public std::exception{
	public:
		virtual const char* what() const throw();
	};
	~Server();
};
