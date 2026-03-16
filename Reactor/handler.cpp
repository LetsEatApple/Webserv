/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:21:43 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/14 20:34:27 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "handler.hpp"

#include "Response.hpp"
#include "reactor.hpp"
#include "ResponseDispatcher.hpp"
#include <sys/socket.h>

//	CONNECTION HANDLER
Connection::Connection(const SessionInfo &session) : EventHandler(session.client.socket),
                                                     _readOffset(0), _session(session),
                                                     _location(), _writeOffset(0),
                                                     _routed(false), _closeAfterResponse(false),
                                                     _headerBytes(0), _lastActivity(std::time(NULL)),
_state(StateReadingHeaders), _cgi(NULL) {
}

Connection::~Connection() {
  _state = StateClosing;
  stopCgi();
}

int Connection::sendErrorAndClose(Response::StatusCode code) {
  _closeAfterResponse = true;
  _writeBuffer.clear();
  _writeOffset = 0;
  Response::Writer w(_writeBuffer);
  ResponseDispatcher(w, _request, _location).sendError(code);
  // Fix connection header: we're closing, not keeping alive
  {
    const std::string old_h = "connection: keep-alive\r\n";
    const std::string new_h = "connection: close\r\n";
    size_t pos = _writeBuffer.find(old_h);
    if (pos != std::string::npos)
      _writeBuffer.replace(pos, old_h.size(), new_h);
  }
  _state = StateSendingResponse;
  return _reactor->modifyHandler(this, EPOLLOUT);
}

void Connection::stopCgi() {
  if (_cgi) {
    _cgi->stop();
    delete _cgi;
    _cgi = NULL;
  }
}

const Request& Connection::request() const { return _request; }
const config::Location& Connection::location() const { return _location; }
std::string& Connection::writeBuffer() { return _writeBuffer; }
size_t Connection::writePending() const { return _writeBuffer.size() - _writeOffset; }
bool Connection::writeEmpty() const { return _writeOffset >= _writeBuffer.size(); }
Reactor* Connection::reactor() { return _reactor; }
ConnState Connection::state() const { return _state; }
void Connection::setState(ConnState s) { _state = s; }
void Connection::setCloseAfterResponse(bool v) { _closeAfterResponse = v; }

uint16_t Connection::serverPort() const {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  if (getsockname(_fd, reinterpret_cast<struct sockaddr*>(&addr), &len) == 0)
    return ntohs(addr.sin_port);
  return 0;
}

bool Connection::checkTimeout() {
  std::time_t now = std::time(NULL);
  if ((_state == StateCgiRunning || _state == StateCgiStreaming) && _cgi && _cgi->startTime() > 0
      && now - _cgi->startTime() >= CGI_TIMEOUT_SEC) {
    logInfo("CGI timeout after ", str(CGI_TIMEOUT_SEC).append("s - killing process"));
    stopCgi();
    sendErrorAndClose(Response::StatusGatewayTimeout);
    return false;
  }
  if (_state == StateSendingResponse || _state == StateCgiRunning || _state == StateCgiStreaming)
    return false;
  if (now - _lastActivity >= CLIENT_TIMEOUT_SEC) {
    logInfo("Client timeout after ", str(CLIENT_TIMEOUT_SEC).append("s of inactivity"));
    sendErrorAndClose(Response::StatusRequestTimeout);
    return false;
  }
  return false;
}

void Connection::compactReadBuffer() {
	if (_readOffset >= MAX) {
		_readBuffer.erase(0, _readOffset);
		_readOffset = 0;
	}
}

ProcessResult Connection::processRequest(){
	std::string errorMsg;
	int consumed = _request.parse(_readBuffer.substr(_readOffset), errorMsg);
	if (consumed < 0)
		return (errorMsg == "Payload Too Large") ? PayloadTooLarge : RequestError;
	if (consumed > 0) {
		_readOffset += consumed;
		compactReadBuffer();
	}
	if (_request.headersDone() && !_routed){
		if (!findLocation(_session.vhosts, _request, _location))
			return RequestError;
		_routed = true;
		_request.setMaxBodySize(_location.client_max_body_size);
		if (_request.checkState(ParserState::Body)
			&& _request.getContentLen() > _location.client_max_body_size)
			return PayloadTooLarge;
		if (!isMethodAllowed(_request.getMethod(), _location.limit_except))
			return MethodNotAllowed;
		if (isCGI(_request.getPath(), _location.cgi)) {
			_cgi = new CgiProcess(*this);
			if (!_cgi->start()) {
				stopCgi();
				return RequestError;
			}
			_state = StateCgiRunning;
		}
		if (_readOffset < _readBuffer.size() && !_request.done()){
			consumed = _request.parse(_readBuffer.substr(_readOffset), errorMsg);
			if (consumed < 0)
				return (errorMsg == "Payload Too Large") ? PayloadTooLarge : RequestError;
			if (consumed > 0) {
				_readOffset += consumed;
				compactReadBuffer();
			}
		}
	}
	if (_cgi && !_request.getBody().empty())
		_cgi->feedBody(_request.bodyRef());
	if (_request.done()) {
		if (_cgi) {
			_cgi->notifyBodyComplete();
			return NeedMoreData;
		}
		return RequestDone;
	}
	return NeedMoreData;
}

int Connection::handleRead(){

  if (_state == StateCgiRunning || _state == StateCgiStreaming) {
    char buf[MAX];
    ssize_t n = recv(_fd, buf, MAX - 1, MSG_DONTWAIT);
    if (n == 0) {
      stopCgi();
      logDisconnect(_session.listener, _session.client);
      return (_reactor->removeHandler(this, 0));
    }
    if (n > 0) {
      _lastActivity = std::time(NULL);
      if (!_request.done()) {
        _readBuffer.append(buf, n);
        std::string errorMsg;
        int consumed = _request.parse(_readBuffer.substr(_readOffset), errorMsg);
        if (consumed < 0) {
          stopCgi();
          return sendErrorAndClose(errorMsg == "Payload Too Large"
            ? Response::StatusPayloadTooLarge : Response::StatusBadRequest);
        }
        if (consumed > 0) {
          _readOffset += consumed;
          compactReadBuffer();
        }
        if (_cgi && !_request.getBody().empty())
          _cgi->feedBody(_request.bodyRef());
        if (_request.done() && _cgi)
          _cgi->notifyBodyComplete();
      }
    }
    return true;
  }

	char buf[MAX];
	ssize_t n = recv(_fd, buf, MAX - 1, MSG_DONTWAIT);
	if (n == 0){ // client disconnected
    stopCgi();
		logDisconnect(_session.listener, _session.client);
		return (_reactor->removeHandler(this, 0));
	}
	if (n > 0) {
		_readBuffer.append(buf, n);
		_lastActivity = std::time(NULL);
	} else if (_readOffset >= _readBuffer.size()) // Error occured and buffer empty, epoll decides what to do
		return false;
	if (_state == StateReadingHeaders)
		_headerBytes += (n > 0) ? n : 0;
	if (_state == StateReadingHeaders && _headerBytes > MAX_HEADER_SIZE)
		return sendErrorAndClose(Response::StatusBadRequest);
	const ProcessResult result = processRequest();

	switch (result){
		case RequestError:
			return sendErrorAndClose(Response::StatusBadRequest);
		case PayloadTooLarge:
			return sendErrorAndClose(Response::StatusPayloadTooLarge);
		case MethodNotAllowed:
			return sendErrorAndClose(Response::StatusMethodNotAllowed);
		case RequestDone: {
			Response::Writer w(_writeBuffer);
			if (_request.error())
				return sendErrorAndClose(Response::StatusBadRequest);
			logRequest(_session.listener, _session.client, _request, _location.path);
			ResponseDispatcher(w, _request, _location).serve();
			_state = StateSendingResponse;
			return _reactor->modifyHandler(this, EPOLLOUT);
		}
		case NeedMoreData:
			return true;
	}
	return true;
}


// if send returns 0 -> dead connection remove socket, if -1 -> error but do not decide anything here just wait for epoll to tell whats wrong
// if (n < 0) {
// Do nothing here
 // epoll will report EPOLLERR / EPOLLHUP if it's fatal

int Connection::handleWrite() {

	while (!writeEmpty()) {
		const ssize_t sent = send(_fd, _writeBuffer.c_str() + _writeOffset,
		                          writePending(), MSG_DONTWAIT);
		if (sent > 0) {
			_writeOffset += static_cast<size_t>(sent);
			if (writeEmpty()) {
				_writeBuffer.clear();
				_writeOffset = 0;
			}
		} else if (sent < 0 && errno == EAGAIN)
			break;
		else if (sent < 0)
			return false;
		else
			break;
	}
	if (_state == StateCgiStreaming) {
		if (_cgi && writePending() < MAX_CGI_BUFFER)
			_cgi->resumeRead();
		if (_cgi && _cgi->isRunning()) {
			if (writeEmpty())
				return _reactor->modifyHandler(this, EPOLLIN);
			return true;
		}
	}
	if (!writeEmpty())
		return true;
	if (_closeAfterResponse) {
		stopCgi();
		logDisconnect(_session.listener, _session.client);
		return _reactor->removeHandler(this, 0);
	}
	stopCgi();
	_writeBuffer.clear();
	_writeOffset = 0;
	_request.reset();
	_routed = false;
	_headerBytes = 0;
	_state = StateReadingHeaders;
	if (_readOffset < _readBuffer.size())
		return handleRead();
	return _reactor->modifyHandler(this, EPOLLIN);
}

// ACCEPT HANDLER
Acceptor::Acceptor(const SessionInfo &session) : EventHandler(session.listener.socket), _session(session) {};

int Acceptor::handleRead() {

	struct sockaddr_in c_addr;
	socklen_t c_addrlen;
	std::memset(&c_addr, 0, sizeof(c_addr));
	c_addrlen = sizeof(c_addr);

	for (;;){
		int cfd = accept(_fd, (struct sockaddr *)&c_addr, &c_addrlen);
		if (cfd < 0)
			break; // accept() may return -1 with EAGAIN if no connections are pending (not an error)
		_session.client.portPair = IP_port(c_addr);
		_session.client.socket = cfd;
    /*
		int flags = fcntl(cfd, F_GETFL, 0);
		int ret = fcntl(cfd, F_SETFL, flags | O_NONBLOCK);
		if (ret == -1 || flags == -1){
			close(cfd);
			return false;
		}
    */
		Connection* conn = new Connection(this->_session);
		if (!_reactor->registerHandler(conn, EPOLLIN)) {
			close(cfd);
			delete conn;
			return false;
		}
		logConnection(_session.listener, _session.client);
	}
	return true;
}

int Acceptor::handleWrite() {
	return true;
}

// SIGNAL HANDLER
Signal::Signal(int fd) : EventHandler(fd){
	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
};

int Signal::handleRead(){

	char buffer[MAX];
  read(_fd, buffer, sizeof(buffer));
	//while (read(_fd, buffer, sizeof(buffer)) > 0) {}
	std::cout << "\n\033[95mSignal detected: Shutting down ...\033[0;37m" << std::endl;
	return (_reactor->stop(SET));
}

int Signal::handleWrite(){
	return true;
}


// STDIN HANDLER
Stdin::Stdin(int fd) : EventHandler(fd){}

int Stdin::handleRead(){

	char buffer[MAX];
	ssize_t size = read(STDIN_FILENO, buffer, MAX - 1);
	if (size <= 0)
		return true;
	std::string input(buffer, size);
	if (!input.empty() && input[input.size() - 1] == '\n')
		input.erase(input.size() - 1);
	if (input == "quit"){
		std::cout << "\033[95mQuit called: Shutting down ..." << std::endl;
		_reactor->stop(SET);
	}
	return true;
}

int Stdin::handleWrite(){
	return true;
}
