/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:21:43 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/24 16:20:18 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "handler.hpp"
#include "reactor.hpp"

//	CONNECTION HANDLER
Connection::Connection(int fd) : EventHandler(fd){};

int Connection::handleRead() {
	//read 
	data.buflen = recv(_fd, data.buffer, MAX-1, MSG_DONTWAIT); // flag similar to O_NONBLOCK of fcntl()
	if (data.buflen <= 0){ // 0 close (client disconnected) <0 Error
		std::cout << "Error: Client disconnected ; " << std::endl;
		if (!reactor->removeHandler(this, 0))
			return false;
	}
	if (data.buflen != -1){
		data.request.append(data.buffer, data.buflen); 
		for (;;){
			size_t delimeter = data.request.find("\n\n"); // replace later with request header delimeter "\r\n\r\n"
			if (delimeter != std::string::npos){
			//process request
				std::string single_request = data.request.substr(0, delimeter +2);
				data.request.erase(0, delimeter +2);
				std::stringstream ss;
				ss << "Client message was: \"" << single_request << "\"\n";
				data.response.append(ss.str());
				int sent = send(this->get_fd(), data.response.c_str(), data.response.length(), MSG_DONTWAIT);
				if (sent < 0){
					/* if (!reactor->removeHandler(this, 0))
						return false; */
					return true;
				}
				if (static_cast<size_t>(sent) < data.response.length()){
					data.response.erase(0, sent);
					if (!reactor->modifyHandler(this, EPOLLIN | EPOLLOUT))
						return false;
					break;
				}
				data.response.erase(0, sent);
			}
			else 
				break;
		}
	}
	return true;
}

int Connection::handleWrite() {

	if (data.response.empty()) {
		reactor->modifyHandler(this, EPOLLIN);
		return true;
	}
	int sent = send(_fd, data.response.c_str(), data.response.length(), MSG_DONTWAIT);
	if (sent > 0)
		data.response.erase(0, sent);
	if (data.response.empty()) {
		reactor->modifyHandler(this, EPOLLIN);
	}
	if (sent <= 0){
		if (!reactor->removeHandler(this, 0))
			return false;
	}
	// if 0 close
	// but if < 0 depending on error
	// if sent <= 0 → assume socket cannot send more now, wait for next EPOLLOUT
	// => modify to epollin?
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
		if (!reactor->registerHandler(new Connection(cfd), EPOLLIN))
			return true;
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
	std::string input;
	std::cin >> input;
	if (input.compare("quit")){
		std::cout << "quit called" << std::endl;
		exit(1);
	}
	return true;
}

int Stdin::handleWrite(){
	return true;
}