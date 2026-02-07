/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:20:30 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/31 21:11:19 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.h"
#include "AEventHandler.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"

typedef std::pair< int, std::vector<config::Server> > ListenerSocket;

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
	Router&					_requestProcessor;
	Data					_data;
	ListenerSocket			_ls;
public:
	Connection(int fd, const ListenerSocket &ls, Router& requestProcessor);
	int handleRead();
	int handleWrite();
	~Connection();
};

// handling listening sockets
class Acceptor : public EventHandler{
private:
	ListenerSocket	_ls;
	Router*			_requestProcessor;
public:
	Acceptor(const ListenerSocket &ls);
	int handleRead();
	int handleWrite();
	~Acceptor();
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