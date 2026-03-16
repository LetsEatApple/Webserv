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
#include "Cgi.hpp"

class Reactor;

enum ConnState {
  StateReadingHeaders,
  StateReadingBody,
  StateCgiRunning,
  StateCgiStreaming,
  StateSendingResponse,
  StateClosing
};

// handling client sockets
class Connection : public EventHandler{

private:
	Request					_request;
	std::string				_readBuffer;
	std::string				_writeBuffer;
	SessionInfo				_session;
	config::Location		_location;
	size_t					_writeOffset;
	bool					_routed;
	bool					_closeAfterResponse;
	size_t					_headerBytes;
	std::time_t				_lastActivity;

  ConnState _state;
  CgiProcess* _cgi;

  void stopCgi();
  int sendErrorAndClose(Response::StatusCode code);
	ProcessResult	processRequest();
public:
	Connection(const SessionInfo &session);
  ~Connection();
	int	handleRead();
	int	handleWrite();
  bool checkTimeout();

  const Request& request() const;
  const config::Location& location() const;
  std::string& writeBuffer();
  size_t writePending() const;
  bool writeEmpty() const;
  Reactor* reactor();
  ConnState state() const;
  void setState(ConnState s);
  void setCloseAfterResponse(bool v);
  uint16_t serverPort() const;
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
