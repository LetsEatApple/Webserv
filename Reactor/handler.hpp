/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:20:30 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/22 16:19:52 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.h"
#include "AEventHandler.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "RequestHandler.hpp"

class Reactor;

// handling client sockets with HTTP protocol
class Connection : public EventHandler{

private:
	Request			_request;
	std::string		_readBuffer;
	RequestHandler*	_handler;
public:
	Connection(int fd, RequestHandler* handler);
	int handleRead();
	int handleWrite();
};

// handling listening sockets
class Acceptor : public EventHandler{

private:
	RequestHandler*	_handler;
public:
	Acceptor(int fd, RequestHandler* handler);
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