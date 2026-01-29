/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reactor.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:14:49 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/25 15:48:21 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lib.h"
#include "reactor.hpp"

static int g_signal_fd = -1;

void on_signal(int sig) {

	(void) sig;
	int savedErrno = errno;
	write(g_signal_fd, "x", 1);
	errno = savedErrno;
};

int set_signalfd(){

	int pfd[2];
	if (pipe(pfd) == -1)
		return -1;
	int ret1 = fcntl(pfd[0], F_SETFL, O_NONBLOCK); // read end, watched by epoll
	int ret2 = fcntl(pfd[1], F_SETFL, O_NONBLOCK); // write end, signalHandler writes here
	if (ret1 == -1 || ret2 == -1)
		return -1;
	g_signal_fd = pfd[1];
	return pfd[0];
}

Reactor::Reactor(Server &server) : _epfd(-1), _server(server), _sig(false){}

int Reactor::init(){

	// epoll_create
	_epfd = epoll_create(42);
	if (_epfd < 0)
		return error(4);
	_ev.events = EPOLLIN;

	// set fd for signals and register handler
	int sfd = set_signalfd();
	if (sfd == -1)
		return error(5);
	if (!registerHandler(new Signal(sfd), EPOLLIN))
		return false;

	// stdin
	if(!registerHandler(new Stdin(STDIN_FILENO), EPOLLIN))
		return false;

	// register listening sockets and their handlers
	for (int i = 0; i < _server.getNports(); i++){
		if (!registerHandler(new Acceptor(_server.getServerList()[i].sock, _server.getRequestHandler()), EPOLLIN))
			return error(6);
	}
	return true;
}

int Reactor::registerHandler(EventHandler *handler, uint32_t event){

	epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.data.fd = handler->get_fd();
	handler->reactor = this;
	ev.events = event;

	if (epoll_ctl(_epfd, EPOLL_CTL_ADD, ev.data.fd, &ev) < 0){
		return false; //error handling? differs between listening/signal and client sockets
	}

	_handlers[ev.data.fd] = handler;
	return true;
}

int Reactor::modifyHandler(EventHandler *handler, uint32_t event){

	epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.data.fd = handler->get_fd();
	ev.events = event;
	if (epoll_ctl(_epfd, EPOLL_CTL_MOD, ev.data.fd, &ev) < 0)
		return false; //error handling
	return true;
}

int Reactor::removeHandler (EventHandler *handler, uint32_t event){

	(void) event;

	if (handler->get_fd() != -1)
		close (handler->get_fd());

	if (epoll_ctl(_epfd, EPOLL_CTL_DEL, handler->get_fd(), NULL) == -1)
		return false;

	delete _handlers[handler->get_fd()];
	_handlers.erase(handler->get_fd());
	return true;
}

int Reactor::handleEvents(){

	int nfds = epoll_wait(_epfd, _evlist, MAX, -1);
	if (nfds < 0)
		return false;

	for (int i = 0; i < nfds; i++){
		if (_evlist[i].events & (EPOLLHUP | EPOLLERR)){
			removeHandler(_handlers[_evlist[i].data.fd], 0);
			continue;
		}
		if (_evlist[i].events & EPOLLIN){
			_handlers[_evlist[i].data.fd]->handleRead();
		}
		if (_evlist[i].events & EPOLLOUT)
			_handlers[_evlist[i].data.fd]->handleWrite();
	}
	return true;
}

bool Reactor::stop(int mode){

	if (mode == SET)
		_sig = true;

	return _sig;
}

Reactor::~Reactor(){

	std::map<int, EventHandler *>::iterator it;

	if (!_handlers.empty()){
		for (int i = 0; i < _server.getNports(); i++)
		_server.getServerList()[i].sock = -1;
	}

	for (it = _handlers.begin(); it != _handlers.end(); ++it){
		close (it->first); // close fd
		delete (it->second); // delete handler
	}
	_handlers.clear();

	if (g_signal_fd != -1)
		close(g_signal_fd); // close writing and of signal pipe
	if (_epfd != -1)
		close(_epfd); // close epoll instance
}
