/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:21:43 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/25 16:13:37 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "handler.hpp"
#include "reactor.hpp"

//	CONNECTION HANDLER
Connection::Connection(int fd) : EventHandler(fd){};

int process(Data &data, int fd){

	data.request.append(data.buffer, data.buflen);
	for (;;){
		size_t delimeter = data.request.find("\n\n"); // replace later with request header delimeter "\r\n\r\n"
		if (delimeter == std::string::npos)
			break;
		std::string single_request = data.request.substr(0, delimeter +2);
		data.request.erase(0, delimeter +2);
		std::stringstream ss;
		ss << "Client request recieved (\"" << single_request << "\")\n";
		data.response.append(ss.str());
		ssize_t sent = send(fd, data.response.c_str(), data.response.length(), MSG_DONTWAIT);
		if (sent < 0)
			return true; //epoll decides what to do
		if (static_cast<size_t>(sent) < data.response.length()){ //kernel buffer full
			data.response.erase(0, sent);
			return false; // modify handler
		}
		data.response.erase(0, sent);
	}
	return true;
}

int Connection::handleRead() {

	//read 
	data.buflen = recv(_fd, data.buffer, MAX-1, MSG_DONTWAIT); // flag similar to O_NONBLOCK of fcntl()
	if (data.buflen == 0) // client disconnected
		return (_reactor->removeHandler(this, 0));
	if (data.buflen < 0) // Error occured, epoll decides what to do 
		return false;
	if (!process(data, this->get_fd()))
		return (_reactor->modifyHandler(this, EPOLLIN | EPOLLOUT));
	return true;
}

// if send returns 0 -> dead connection remove socket, if -1 -> error but do not decide anything here just wait for epoll to tell whats wrong
// if (n < 0) {
// Do nothing here
 // epoll will report EPOLLERR / EPOLLHUP if it's fatal

int Connection::handleWrite() {

	if (data.response.empty())
		return (_reactor->modifyHandler(this, EPOLLIN));

	int sent = send(_fd, data.response.c_str(), data.response.length(), MSG_DONTWAIT);
	if (sent > 0)
		data.response.erase(0, sent);
	if (data.response.empty())
		return (_reactor->modifyHandler(this, EPOLLIN));
	if (sent < 0)
		return false; // decision up to epoll; Wait for epoll to report EPOLLERR / EPOLLHUP

	return true;
}

// ACCEPT HANDLER
Acceptor::Acceptor(int fd) : EventHandler(fd){};

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
		if (!_reactor->registerHandler(new Connection(cfd), EPOLLIN)) // close program?
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