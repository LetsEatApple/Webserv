/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:21:43 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/25 15:49:14 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "handler.hpp"
#include "reactor.hpp"

//	CONNECTION HANDLER
Connection::Connection(int fd, RequestHandler* rhandler)
	: EventHandler(fd), _requestHandler(rhandler){}

int Connection::handleRead() {

	char buf[MAX];
	ssize_t n = recv(_fd, buf, MAX - 1, MSG_DONTWAIT);
	if (n == 0) // client disconnected
		return (reactor->removeHandler(this, 0));
	if (n < 0) // Error occured, epoll decides what to do
		return false;

	_readBuffer.append(buf, n);

	std::string errorMsg;
	int consumed = _request.parse(_readBuffer, errorMsg);
	if (consumed < 0) {
		Response::Writer w(_fd);
		w.writeStatusLine(Response::StatusBadRequest);
		w.writeHeaders(Response::getDefaultHeaders(0));
		return (reactor->removeHandler(this, 0));
	}
	if (consumed > 0)
		_readBuffer.erase(0, consumed);
	if (_request.done()) {
		Response::Writer w(_fd);
		if (_request.error()) {
			w.writeStatusLine(Response::StatusBadRequest);
			w.writeHeaders(Response::getDefaultHeaders(0));
		} else {
			_requestHandler->handle(w, _request);
		}
		return (reactor->removeHandler(this, 0));
	}
	return true;
}

int Connection::handleWrite() {
	return true;
}

// ACCEPT HANDLER
Acceptor::Acceptor(int fd, RequestHandler* requestHandler) : EventHandler(fd), _requestHandler(requestHandler){};

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
		if (ret == -1 || flags == -1)
			return false;
		if (!reactor->registerHandler(new Connection(cfd, _requestHandler), EPOLLIN))
			return false;
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

	char buf[16];
	while (read(_fd, buf, sizeof(buf)) > 0) {}
	std::cout << "\nSignal detected: quitting program ..." << std::endl;
	reactor->stop(SET);
	return true;
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
		reactor->stop(SET);
	}
	return true;
}

int Stdin::handleWrite(){
	return true;
}
