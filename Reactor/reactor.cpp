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

#include "AEventHandler.hpp"
#include "constants.hpp"
#include "reactor.hpp"

#include <cerrno>
#include <csignal>
#include <Logger.hpp>
#include <map>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>

static int g_signal_fd = -1;

void on_signal(int sig) {
	(void) sig;
	const int savedErrno = errno;
	const ssize_t n = write(g_signal_fd, "x", 1);
	if (n < 0)
		logDebug("write failed", 1);
	errno = savedErrno;
}

int set_signalfd(){
	int pfd[2];
	if (pipe(pfd) == -1)
		return -1;
	g_signal_fd = pfd[1];
	return pfd[0];
}

Reactor::Reactor() : _epfd(-1), _ev(), _evlist(), _sig(false) {}

int Reactor::init(const activeSessions &sessions){

	signal(SIGPIPE, SIG_IGN);

	// epoll_create
	_epfd = epoll_create(42);
	if (_epfd < 0)
		return error("epoll_create failed", 1);
	_ev.events = EPOLLIN;
	logInfo("Reactor initialized: \033[0;36mepoll (fd=", str(_epfd).append(")"));

	// set fd for signals and register handler
	const int sfd = set_signalfd();
	if (sfd == -1)
		return error("creating signal fd failed", 1);
	Signal* sig = new Signal(sfd);
	if (!registerHandler(sig, EPOLLIN)) {
		delete sig;
		return false;
	}

	// stdin
	Stdin* in = new Stdin(STDIN_FILENO);
	if(!registerHandler(in, EPOLLIN)) {
		delete in;
		return false;
	}

	// register listening sockets and their handlers
	for (size_t i = 0; i < sessions.size(); i++){
		Acceptor* acc = new Acceptor(sessions[i]);
		if (!registerHandler(acc, EPOLLIN)) {
			delete acc;
			return error("epoll_ctl on listening socket failed", 1);
		}
	}
	logInfo("Server ready to accept connections", " ...");
	return true;
}

int Reactor::registerHandler(EventHandler *handler, const uint32_t event){

	epoll_event ev = {};
	ev.data.fd = handler->get_fd();
	handler->reactor(this);
	ev.events = event;
	
	if (epoll_ctl(_epfd, EPOLL_CTL_ADD, ev.data.fd, &ev) < 0)
		return false;

	_handlers[ev.data.fd] = handler;
	return true;
}

int Reactor::modifyHandler(const EventHandler *handler, const uint32_t event){

	epoll_event ev = {};
	ev.data.fd = handler->get_fd();
	ev.events = event;
	if (epoll_ctl(_epfd, EPOLL_CTL_MOD, ev.data.fd, &ev) < 0){
		logDebug("modifying handler (epoll_ctl) failed", 1);
		removeHandler(handler, event);
		return false;
	}
	return true;
}

int Reactor::removeHandler (const EventHandler *handler, const uint32_t event){

	(void) event;
	const int fd = handler->get_fd();

	if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
		logDebug("Epoll_ctl failed", 1);

	if (fd != -1)
		close (fd);

	delete handler;
	_handlers.erase(fd);
	return true;
}

void Reactor::handleEvents(){

	const int nfds = epoll_wait(_epfd, _evlist, MAX, 1000);
	if (nfds == 0) {
		checkTimeouts();
		return;
	}

	for (int i = 0; i < nfds; i++){
		int fd = _evlist[i].data.fd;
		if (_handlers.find(fd) == _handlers.end())
			continue;

		const uint32_t ev = _evlist[i].events;

		if (ev & (EPOLLIN | EPOLLHUP | EPOLLERR))
			_handlers[fd]->handleRead();
		if ((ev & EPOLLOUT)
			&& _handlers.find(fd) != _handlers.end())
			_handlers[fd]->handleWrite();
	}
	checkTimeouts();
}

void Reactor::checkTimeouts() {
  std::vector<EventHandler*> timedOut;
  for (std::map<int, EventHandler*>::iterator it = _handlers.begin(); it != _handlers.end(); ++it) {
	if (it->second->checkTimeout())
	  timedOut.push_back(it->second);
  }
  for (size_t i = 0; i < timedOut.size(); ++i) {
	int fd = timedOut[i]->get_fd();
	if (_handlers.find(fd) != _handlers.end())
	  removeHandler(timedOut[i], 0);
  }
}

bool Reactor::stop(const int mode){

	if (mode == SET)
		_sig = true;

	return _sig;
}

Reactor::~Reactor(){
	for (std::map<int, EventHandler*>::iterator it = _handlers.begin(); it != _handlers.end(); ++it){
		close (it->first); // close fd
		delete (it->second); // delete handler
	}
	_handlers.clear();

	if (g_signal_fd != -1)
		close(g_signal_fd); // close writing end of signal pipe
	if (_epfd != -1)
		close(_epfd); // close epoll instance
}

