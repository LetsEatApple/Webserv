/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:20:30 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/14 19:08:40 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.hpp"
#include "AEventHandler.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "server.hpp"
#include "routing.hpp"


class Reactor;

// handling client sockets
class Connection : public EventHandler{

private:
	Request					_request;
	std::string				_readBuffer;
	std::string				_writeBuffer;
	SessionInfo				_session;
	config::Location		_location;
	bool					_routed;
	bool					_closeAfterResponse;

	ProcessResult	processRequest();
public:
	Connection(const SessionInfo &session);
	int	handleRead();
	int	handleWrite();
};

// handling listening sockets
class Acceptor : public EventHandler{
private:
	SessionInfo	_session;
public:
	Acceptor(const SessionInfo &session);
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
