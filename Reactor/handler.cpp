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
#include "routing.hpp"
#include "reactor.hpp"
#include "ResponseDispatcher.hpp"

//	CONNECTION HANDLER
Connection::Connection(const SessionInfo &session) : EventHandler(session.client.socket), _session(session),
                                                     _location(),
                                                     _routed(false), _closeAfterResponse(false) {
}

ProcessResult Connection::processRequest(){
	std::string errorMsg;
	int consumed = _request.parse(_readBuffer, errorMsg);
	if (consumed < 0)
		return RequestError;
	if (consumed > 0)
		_readBuffer.erase(0, consumed);
	if (_request.headersDone() && !_routed){
		if (!findLocation(_session.vhosts, _request, _location))
			return RequestError;
		_routed = true;
		_request.setMaxBodySize(_location.client_max_body_size);
		if (!isMethodAllowed(_request.getMethod(), _location.limit_except))
			return MethodNotAllowed;
		if (_request.checkState(ParserState::Body)
			&& _request.getContentLen() > _location.client_max_body_size)
			return PayloadTooLarge;
		// TODO: if CGI, fork here and pipe body to stdin as it arrives
		if (isCGI(_request.getPath(), _location.cgi)) {
			// TODO: forkCGI()
		}
		if (!_readBuffer.empty() && !_request.done()){
			consumed = _request.parse(_readBuffer, errorMsg);
			if (consumed < 0)
				return RequestError;
			if (consumed > 0)
				_readBuffer.erase(0, consumed);
		}
	}
	if (_request.done())
		return RequestDone;
	return NeedMoreData;
}

int Connection::handleRead(){

	char buf[MAX];
	ssize_t n = recv(_fd, buf, MAX - 1, MSG_DONTWAIT);
	if (n == 0){ // client disconnected
		logDisconnect(_session.listener, _session.client);
		return (_reactor->removeHandler(this, 0));
	}
	if (n > 0)
		_readBuffer.append(buf, n);
	else if (_readBuffer.empty()) // Error occured and buffer empty, epoll decides what to do
		return false;
	const ProcessResult result = processRequest();

	switch (result){
		case RequestError: {
			_closeAfterResponse = true;
			Response::Writer w(_writeBuffer);
			ResponseDispatcher(w, _request, _location).sendError(Response::StatusBadRequest);
			return _reactor->modifyHandler(this, EPOLLOUT);
		}
		case PayloadTooLarge: {
			_closeAfterResponse = true;
			Response::Writer w(_writeBuffer);
			ResponseDispatcher(w, _request, _location).sendError(Response::StatusPayloadTooLarge);
			return _reactor->modifyHandler(this, EPOLLOUT);
		}
		case RequestDone: {
			Response::Writer w(_writeBuffer);
			if (_request.error()) {
				_closeAfterResponse = true;
				ResponseDispatcher(w, _request, _location).sendError(Response::StatusBadRequest);
			} else {
				logRequest(_session.listener, _session.client, _request, _location.path);
				ResponseDispatcher(w, _request, _location).serve();
			}
			return _reactor->modifyHandler(this, EPOLLOUT);
		}
		case MethodNotAllowed: {
			_closeAfterResponse = true;
			Response::Writer w(_writeBuffer);
			ResponseDispatcher(w, _request, _location).sendError(Response::StatusMethodNotAllowed);
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

	if (_writeBuffer.empty()) {
		if (_closeAfterResponse) {
			logDisconnect(_session.listener, _session.client);
			return _reactor->removeHandler(this, 0);
		}
		_request.reset();
		_routed = false;
		if (!_readBuffer.empty())
			return handleRead();
		return (_reactor->modifyHandler(this, EPOLLIN));
	}

	const ssize_t sent = send(_fd, _writeBuffer.c_str(), _writeBuffer.length(), MSG_DONTWAIT);
	if (sent > 0)
		_writeBuffer.erase(0, sent);
	if (_writeBuffer.empty()) {
		if (_closeAfterResponse) {
			logDisconnect(_session.listener, _session.client);
			return _reactor->removeHandler(this, 0);
		}
		_request.reset();
		_routed = false;
		if (!_readBuffer.empty())
			return handleRead();
		return (_reactor->modifyHandler(this, EPOLLIN));
	}
	if (sent < 0)
		return false; // decision up to epoll; Wait for epoll to report EPOLLERR / EPOLLHUP

	return true;
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
		int flags = fcntl(cfd, F_GETFL, 0);
		int ret = fcntl(cfd, F_SETFL, flags | O_NONBLOCK);
		if (ret == -1 || flags == -1){
			close(cfd);
			return false;
		}
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
	while (read(_fd, buffer, sizeof(buffer)) > 0) {}
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