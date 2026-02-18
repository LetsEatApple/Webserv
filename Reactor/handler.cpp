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
Connection::Connection(const SessionInfo &session) : EventHandler(session.client.socket), _session(session), _routed(false){}

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
	if (n < 0) // Error occured, epoll decides what to do
		return false;

	_readBuffer.append(buf, n);
	ProcessResult result = processRequest();

	switch (result){
		case RequestError: {
			Response::Writer w(_fd);
			w.writeStatusLine(Response::StatusBadRequest);
			w.writeHeaders(Response::getDefaultHeaders(0));
			logDisconnect(_session.listener, _session.client);
			return (_reactor->removeHandler(this, 0));
		}
		case PayloadTooLarge: {
			Response::Writer w(_fd);
			w.writeStatusLine(Response::StatusPayloadTooLarge);
			w.writeHeaders(Response::getDefaultHeaders(0));
			logDisconnect(_session.listener, _session.client);
			return _reactor->removeHandler(this, 0);
		}
		case RequestDone: {
			Response::Writer w(_fd);
			if (_request.error()) {
				w.writeStatusLine(Response::StatusBadRequest);
				w.writeHeaders(Response::getDefaultHeaders(0));
			} else {
				logRequest(_session.listener, _session.client, _request, _location.path);
				ResponseDispatcher(w, _request, _location).serve();
			}
			logDisconnect(_session.listener, _session.client);
			return (_reactor->removeHandler(this, 0));
		}
		case MethodNotAllowed: {
			Response::Writer w(_fd);
			w.writeStatusLine(Response::StatusMethodNotAllowed);
			w.writeHeaders(Response::getDefaultHeaders(0));
			logDisconnect(_session.listener, _session.client);
			return (_reactor->removeHandler(this, 0));
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

	if (_data.response.empty())
		return (_reactor->modifyHandler(this, EPOLLIN));

	int sent = send(_fd, _data.response.c_str(), _data.response.length(), MSG_DONTWAIT);
	if (sent > 0)
		_data.response.erase(0, sent);
	if (_data.response.empty())
		return (_reactor->modifyHandler(this, EPOLLIN));
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
		if (!_reactor->registerHandler(new Connection(this->_session), EPOLLIN)) // close program?
			return false;
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