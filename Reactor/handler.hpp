/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:20:30 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/28 20:42:38 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.h"
#include "AEventHandler.hpp"

class Reactor;

// storing read and write data for request and response
struct Data{
	std::string	request;
	int			max_size;
	std::string	response;
	char		buffer[MAX];
	int			buflen;
};

// handling client sockets
class Connection : public EventHandler{

private:
	Data					_data;
	int						_lfd;
	config::ListenerData	_httpServer;
	//adding struct containing data from config depending on fd / listening fd? or through reactor _server?
public:
	Connection(int fd, Reactor *reactor, int lfd);
	int handleRead();
	int handleWrite();
};

// handling listening sockets
class Acceptor : public EventHandler{

public:
	Acceptor(int fd, Reactor *reactor);
	int handleRead();
	int handleWrite();
};

// handling signal pipe
class Signal : public EventHandler{

public:
	Signal(int fd, Reactor *reactor);
	int handleRead();
	int handleWrite();
};

// handling standard input
class Stdin : public EventHandler{

public:
	Stdin(int fd, Reactor *reactor);
	int handleRead();
	int handleWrite();
};