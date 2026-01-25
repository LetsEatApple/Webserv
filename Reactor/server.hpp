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

struct server_list{
	int sock;
	int port;
	struct sockaddr_in s_addr;
};


class Server{
private:
	Server();
public:
	Server(int nports, char **ports);
	int s_sock;
	int c_sock;
	std::vector<struct server_list> s_list;
	int nports;
	int createServer();
	~Server();
};
