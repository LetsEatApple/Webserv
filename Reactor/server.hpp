/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:06:56 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/26 17:55:47 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.h"

struct server{
	int sock;
	int port;
	struct sockaddr_in s_addr;
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
	~Server();
};
