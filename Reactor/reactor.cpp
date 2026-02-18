/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reactor.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:14:49 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/14 18:35:17 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lib.hpp"
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
	if (ret1 == -1 || ret2 == -1){
		close(pfd[0]);
		close(pfd[1]);
		return -1;
	}
	g_signal_fd = pfd[1];
	return pfd[0];
}

Reactor::Reactor() : _epfd(-1), _ev(), _evlist(), _sig(false) {}

int Reactor::init(const activeSessions &sessions){

	// epoll_create
	_epfd = epoll_create(42);
	if (_epfd < 0)
		return error("epoll_create failed", 1);
	_ev.events = EPOLLIN;
	logInfo("Reactor initialized: \033[0;36mepoll (fd=", str(_epfd).append(")"));

	// set fd for signals and register handler
	int sfd = set_signalfd();
	if (sfd == -1)
		return error("creatig signal fd failed", 1);
	if (!registerHandler(new Signal(sfd), EPOLLIN))
		return false;

	// stdin
	if(!registerHandler(new Stdin(STDIN_FILENO), EPOLLIN))
		return false;

	// register listening sockets and their handlers
	for (size_t i = 0; i < sessions.size(); i++){
		if (!registerHandler(new Acceptor(sessions[i]), EPOLLIN))
			return error("epoll_ctl on listening socket failed", 1);
	}
	logInfo("Server ready to accept connections", " ...");
	return true;
}

int Reactor::registerHandler(EventHandler *handler, uint32_t event){

	epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.data.fd = handler->get_fd();
	handler->reactor(this);
	ev.events = event;
	
	if (epoll_ctl(_epfd, EPOLL_CTL_ADD, ev.data.fd, &ev) < 0)
		return false;

	_handlers[ev.data.fd] = handler;
	return true;
}

int Reactor::modifyHandler(EventHandler *handler, uint32_t event){

	epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.data.fd = handler->get_fd();
	ev.events = event;
	if (epoll_ctl(_epfd, EPOLL_CTL_MOD, ev.data.fd, &ev) < 0){
		logDebug("modifying handler (epoll_ctl) failed", 1);
		removeHandler(handler, event);
		return false;
	}
	return true;
}

int Reactor::removeHandler (EventHandler *handler, uint32_t event){

	(void) event;
	int fd = handler->get_fd();

	if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
		logDebug("Epoll_ctl failed", 1);

	if (fd != -1)
		close (fd);

	delete handler;
	_handlers.erase(fd);
	return true;
}

int Reactor::handleEvents(){

	int nfds = epoll_wait(_epfd, _evlist, MAX, -1);
	if (nfds < 0)
		return false;

	for (int i = 0; i < nfds; i++){
		int fd = _evlist[i].data.fd;
		if (_handlers.find(fd) == _handlers.end())
			continue;
		if (_evlist[i].events & (EPOLLHUP | EPOLLERR))
			removeHandler(_handlers[fd], 0);
		else if (_evlist[i].events & EPOLLIN)
			_handlers[fd]->handleRead();
		else if (_evlist[i].events & EPOLLOUT)
			_handlers[fd]->handleWrite();
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

	for (it = _handlers.begin(); it != _handlers.end(); ++it){
		close (it->first); // close fd
		delete (it->second); // delete handler
	}
	_handlers.clear();

	if (g_signal_fd != -1)
		close(g_signal_fd); // close writing end of signal pipe
	if (_epfd != -1)
		close(_epfd); // close epoll instance
}

