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

class RequestHandler;

struct server_list{
	int sock;
	int port;
	struct sockaddr_in s_addr;
};


class Server{
private:
	Server();
	RequestHandler*					_handler;
	int								_s_sock;
	int								_c_sock;
	std::vector<struct server_list>	_s_list;
	int								_nports;
public:
	Server(int nports, char **ports, RequestHandler* handler);
	RequestHandler*	getHandler() const;
	int				getNports() const;
	std::vector<struct server_list>&	getServerList();
	int				createServer();
	~Server();
};
