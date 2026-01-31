/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:20:30 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/31 16:52:07 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.h"
#include "AEventHandler.hpp"
#include "../Http/request/Request.hpp"
#include "../Http/response/Response.hpp"
#include "../Http/router/RequestHandler.hpp"

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
	Request					_request;
	std::string				_readBuffer;
	RequestHandler*			_requestHandler;
	Data					_data;
	int						_lfd;
	config::ListenerData	_httpServer;
	//adding struct containing data from config depending on fd / listening fd? or through reactor _server?
public:
	//Connection(int fd, RequestHandler* requestHandler);
	Connection(int fd, int lfd);
	int handleRead();
	int handleWrite();
	~Connection();
};

// handling listening sockets
typedef std::pair<int, config::ListenerData> ListenerSocket;

class Acceptor : public EventHandler{
private:
	ListenerSocket	_ls;
public:
	Acceptor(const ListenerSocket &ls);
	int handleRead();
	int handleWrite();
};

// handling signal pipe
class Signal : public EventHandler{

public:
	Signal(int fd);
	int handleRead();
	int handleWrite();
};

// handling standard input
class Stdin : public EventHandler{

public:
	Stdin(int fd);
	int handleRead();
	int handleWrite();
};