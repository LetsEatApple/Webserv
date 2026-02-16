/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:06:56 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/14 17:37:55 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.hpp"
#include "Config.hpp"

//typedef std::pair< int, std::vector<config::Server> > ListenerSocket;
typedef std::vector<struct server> server_list;

struct Endpoint{
	int				socket;
	std::string		portPair;
};

struct SessionInfo{
	std::vector<config::Server>	vhosts;
	Endpoint					listener;
	Endpoint					client;
};

struct server{
	SessionInfo			session;
	int					sock;
	int					portNum;
	std::string			portStr;
	std::string			host; //addr
	struct addrinfo *	n_server; //network server
};

typedef std::vector<SessionInfo> activeSessions;

class Server{

private:
	activeSessions	_sessions;
	server_list		_servers;
	Server();

public:
	Server(config::Listeners &configlisteners);

	const activeSessions	&get_sessions() const;
	int						createServer();
	int						error_close(const std::string& err, int index);

	class InvalidAddrException : public std::exception{
	public:
		virtual const char* what() const throw();
	};
	~Server();
};
