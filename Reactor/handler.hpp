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
	size_t					_readOffset;
	std::string				_writeBuffer;
	SessionInfo				_session;
	config::Location		_location;
	size_t					_writeOffset;
	bool					_routed;
	bool					_closeAfterResponse;
	std::time_t				_lastActivity;

  ConnState _state;
  CgiProcess* _cgi;
  int _fileFd;
  size_t _fileRemaining;
  bool _wantRead;
  bool _wantWrite;

  void stopCgi();
  void compactReadBuffer();
  int sendErrorAndClose(Response::StatusCode code);
	ProcessResult	processRequest();
public:
	explicit Connection(const SessionInfo &session);
  ~Connection();
	int	handleRead();
	int	handleWrite();
  bool checkTimeout();

  const Request& request() const;
  const config::Location& location() const;
  std::string& writeBuffer();
  size_t writePending() const;
  bool writeEmpty() const;
  Reactor* reactor() const;
  ConnState state() const;
  void setState(ConnState s);
  void setCloseAfterResponse(bool v);
  void setFileResponse(int fd, size_t size);
  void setWantRead(const bool v)  { _wantRead = v; }
  void setWantWrite(const bool v) { _wantWrite = v; }
  int updateEvents() const;
  uint16_t serverPort() const;
};

// handling listening sockets
class Acceptor : public EventHandler{
private:
	SessionInfo	_session;
public:
	explicit Acceptor(const SessionInfo &session);
	int handleRead();
	int handleWrite();
};

// handling signal pipe
class Signal : public EventHandler{

public:
	explicit Signal(int fd);
	int handleRead();
	int handleWrite();
};

// handling standard input
class Stdin : public EventHandler{

public:
	explicit Stdin(int fd);
	int handleRead();
	int handleWrite();
};
