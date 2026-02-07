/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:21:43 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/31 22:33:27 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "handler.hpp"
#include "reactor.hpp"
#include "ReturnDirective.hpp"

//	CONNECTION HANDLER
Connection::Connection(int fd, const ListenerSocket &ls, Router& requestProcessor)
	: EventHandler(fd), _requestProcessor(requestProcessor), _ls(ls) {}

int Connection::handleRead() {
	char buf[MAX];
	ssize_t n = recv(_fd, buf, MAX - 1, MSG_DONTWAIT);
	if (n == 0) // client disconnected
		return (_reactor->removeHandler(this, 0));
	if (n < 0) // Error occured, epoll decides what to do
		return false;

	
	_readBuffer.append(buf, n);
	std::string errorMsg;
	int consumed = _request.parse(_readBuffer, errorMsg);
	if (consumed < 0) {
		Response::Writer w(_fd);
		w.writeStatusLine(Response::StatusBadRequest);
		w.writeHeaders(Response::getDefaultHeaders(0));
		return (_reactor->removeHandler(this, 0));
	}
	if (consumed > 0)
		_readBuffer.erase(0, consumed);
	if (_request.done()) {
#ifdef DEBUG
		std::cout << _request.getMethod() << " " << _request.getTarget() << std::endl;
#endif
		Response::Writer w(_fd);
		if (_request.error()) {
			w.writeStatusLine(Response::StatusBadRequest);
			w.writeHeaders(Response::getDefaultHeaders(0));
		} else if (!serveReturn(w, _request, _ls.second)) {
			_requestProcessor.process(w, _request);
		}
		return (_reactor->removeHandler(this, 0));
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

Connection::~Connection() {}

// ACCEPT HANDLER
Acceptor::Acceptor(const ListenerSocket &ls) : EventHandler(ls.first), _ls(ls), _requestProcessor(new Router()) {};

int Acceptor::handleRead() {

	struct sockaddr_in c_addr;
	socklen_t c_addrlen;
	std::memset(&c_addr, 0, sizeof(c_addr));
	c_addrlen = sizeof(c_addr);

	for (;;){
		int cfd = accept(_fd, (struct sockaddr *)&c_addr, &c_addrlen);
		if (cfd < 0)
			break; // accept() may return -1 with EAGAIN if no connections are pending (not an error)
		int flags = fcntl(cfd, F_GETFL, 0);
		int ret = fcntl(cfd, F_SETFL, flags | O_NONBLOCK);
		if (ret == -1 || flags == -1) {
			close(cfd);
			return false;
		}
		if (!_reactor->registerHandler(new Connection(cfd, this->_ls, *_requestProcessor), EPOLLIN))
			return false;
	}
	return true;
}

int Acceptor::handleWrite() {
	return true;
}

Acceptor::~Acceptor() {
	delete _requestProcessor;
}

// SIGNAL HANDLER
Signal::Signal(int fd) : EventHandler(fd){
	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
};

int Signal::handleRead(){

	char buffer[MAX];
	while (read(_fd, buffer, sizeof(buffer)) > 0) {}
	std::cout << "\nSignal detected: Shutting down ..." << std::endl;
	return (_reactor->stop(SET));
}

int Signal::handleWrite(){
	return true;
}


// STDIN HANDLER
Stdin::Stdin(int fd) : EventHandler(fd){}

int Stdin::handleRead(){

	char buffer[MAX];
	int	size;
	std::string input;

	size = read(STDIN_FILENO, buffer, MAX-1);
	input.append(buffer, size);
	if (input.compare("quit")){
		std::cout << "Quit called: Shutting down ..." << std::endl;
		_reactor->stop(SET);
	}
	return true;
}

int Stdin::handleWrite(){
	return true;
}