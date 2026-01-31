/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:06:56 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/31 22:25:38 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.h"

typedef std::pair< int, std::vector<config::Server> > ListenerSocket;
typedef std::vector<ListenerSocket> serverListeners;
typedef std::vector<struct server> server_list;

struct server{
	ListenerSocket		ls;
	int					sock;
	int					portNum;
	std::string			portStr;
	std::string			host; //addr
	struct addrinfo *	n_server; //network server
};


class Server{

private:
	serverListeners	_listeners;
	server_list		_slist;
	Server();

public:
	Server(config::Listeners &configlisteners);

	const serverListeners	&get_listeners();
	int						createServer();
	int						error_close(int err, int index);

	class InvalidAddrException : public std::exception{
	public:
		virtual const char* what() const throw();
	};
	~Server();
};
