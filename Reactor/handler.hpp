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
class Connection;

class CgiFdHandler : public EventHandler {
public:
  enum Kind { CGI_IN, CGI_OUT };
  CgiFdHandler(int fd, Connection *owner, Kind kind);
  ~CgiFdHandler();
  int handleRead();
  int handleWrite();
private:
  Connection *_owner;
  Kind _kind;
};

enum ConnState {
  StateReadingHeaders,
  StateReadingBody,
  StateCgiRunning,
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
	bool					_routed;
	bool					_closeAfterResponse;
	size_t					_headerBytes;

  ConnState _state;
  bool _isCgi;
  pid_t _cgiPid;
  int _cgiInFd;
  int _cgiOutFd;
  bool _cgiInClosed;

  std::string _cgiInBuffer;
  std::string _cgiOutBuffer;
  CgiFdHandler* _cgiInHandler;
  CgiFdHandler* _cgiOutHandler;
  std::time_t    _cgiStartTime;

  bool startCgi();
  void stopCgi();
  bool closeCgiInputIfDone();
  bool buildHttpResponseFromCgi();

	ProcessResult	processRequest();
public:
	Connection(const SessionInfo &session);
  ~Connection();
	int	handleRead();
	int	handleWrite();
  bool checkTimeout();
  int onCgiWritable();
  int onCgiReadable();
  void onCgiHandlerClosed(CgiFdHandler::Kind kind);
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
