/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:20:30 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/10 19:29:42 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.hpp"
#include "AEventHandler.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "RequestHandler.hpp"

typedef std::pair< int, std::vector<config::Server> > ListenerSocket;

class Reactor;

// storing read and write data for request and response
struct Data{
	std::string	request;
	int			max_size;
	std::string	response;
	char		buffer[MAX];
	int			buflen;
};

// handling client sockets
typedef std::pair<Request, config::Location>	RequestLocPair;
class Connection : public EventHandler{

private:
	Request					_request;
	std::string				_readBuffer;
	RequestHandler*			_requestHandler;
	ListenerSocket			_ls;
	RequestLocPair			_RequestLocPair;
	Data					_data;
	bool					_routed;
	//adding struct containing data from config depending on fd / listening fd? or through reactor _server?
public:
	//Connection(int fd, RequestHandler* requestHandler);
	Connection(int fd, const ListenerSocket &ls);
	int	handleRead();
	int	handleWrite();
	~Connection();
};

// handling listening sockets
class Acceptor : public EventHandler{
private:
	ListenerSocket	_ls;
public:
	Acceptor(const ListenerSocket &ls);
	int handleRead();
	int handleWrite();
};

// handling signal pipe
class Signal : public EventHandler{

public:
	Signal(int fd);
	int handleRead();
	int handleWrite();
};

// handling standard input
class Stdin : public EventHandler{

public:
	Stdin(int fd);
	int handleRead();
	int handleWrite();
};

bool	getLocation(std::vector<config::Server> servers, Request const request, RequestLocPair *rlp);
int findTarget(config::Server server, std::string path);
std::string removeDoubleSlash(std::string path);
std::string removeDotSegments(std::string input);
std::string	decodePath(std::string target);
bool unreservedChar(char c);
bool matchesLocation(std::string target, std::string location);
int	findHost(std::vector<config::Server> servers, std::string host);