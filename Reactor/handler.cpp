/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handler.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:21:43 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/11 17:21:51 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "handler.hpp"
#include "reactor.hpp"
#include "Router.hpp"
#include "routes.hpp"

//	CONNECTION HANDLER
Connection::Connection(int fd, const ListenerSocket &ls) : EventHandler(fd), _ls(ls), _routed(false){
	/* Router* router = new Router(); //leaking when exiting with signal
	_requestHandler = router;
	// Register middleware
	router->use(logRequest);

	// Register routes
	router->get("/", handleHome);
	router->get("/health", handleHealth);
	router->get("/echo", handleEcho);

	// Register prefix route (matches /api/-*)
	router->prefix("/api", handleApi);

	// Default handler for unmatched routes
	router->setDefault(handleNotFound); */
}

int	findHost(std::vector<config::Server> servers, std::string host){

	for (size_t i = 0; i < servers.size(); i++){
		for (size_t j = 0; j < servers[i].server_names.size(); j++){
			if (servers[i].server_names[j].compare(host) == 0){
				return i;
			}
		}
	}
	return 0;
}

bool matchesLocation(std::string target, std::string location){

	if (target == location)
		return true;
	if (target.size() > location.size() &&
		target.compare(0, location.size(), location) == 0 &&
		target[location.size()] == '/')
		return true;
	return false;
}

bool unreservedChar(char c){

	if ((c >= 48 && c <= 57) ||
		(c >= 65 && c <= 90) || (c >= 97 && c <= 122) ||
		c == 45 || c == 46 || c == 95 || c == 126)
		return true;
	return false;
}

std::string	decodePath(std::string target){

	std::string		hex;
	size_t			found;
	int				dec;

	found = target.find("%");
	while (found != std::string::npos){
		if (found +2 >= target.size())
			return "";
		hex = target.substr(found +1, 2);
		if (!std::isxdigit(hex[0]) || !std::isxdigit(hex[1]))
			return "";
		dec = std::strtoul(hex.c_str(), NULL, 16);
		if (dec <= 0 || dec > 255) // should %00 (hex) = \0 (char) = 0 (dec) invalid
			return "";
		if (unreservedChar(static_cast<char>(dec))){
			target.erase(found, 3);
			target.insert(found, 1, static_cast<char>(dec));
		}
		found = target.find("%", found +1);
	}
	return target;
}

std::string removeDotSegments(std::string input){

	std::string output;

	while(!input.empty()){

		// A: input buffer begins with a prefix of "../" or "./" remove prefix
		if (input.rfind("../", 0) == 0 || input.rfind("./", 0) == 0)
			input.erase(0, input.find('/') +1);

		// B: begins with "/./" or "/." replace with "/"
		else if(input.rfind("/./", 0) == 0 || (input.rfind("/.", 0) == 0 && input.rfind("/..", 0) != 0)){
			if (input.rfind("/./", 0) == 0)
				input.replace(0, 3, "/");
			else
				input.replace(0, 2, "/");
		}

		// C: begins with "/../" or "/.." replace with "/" and remove last segment and its preceding "/" from output
		else if (input.rfind("/../", 0) == 0 || input.rfind("/..", 0) == 0){
			if (input.rfind("/../", 0) == 0)
				input.replace(0, 4, "/");
			else
				input.replace(0, 3, "/");
			size_t found = output.find_last_of('/');
			if (found != std::string::npos)
				output.erase(found);
		}

		// D: if consits only of "." or ".." remove
		else if (input.compare("..") == 0 || input.compare(".") == 0)
			input.clear();

		// E: move first path segment to end of output buffer, including initial "/" and any subsequent chars up to, but not incl next "/" or the end of input
		else{ 
			size_t n = input.find('/', 1);
			if (n != std::string::npos){
				output.append(input, 0, n);
				input.erase(0, n);
			}
			else{
				output.append(input);
				input.clear();
			}
		}
	}
	return output;
}

std::string removeDoubleSlash(std::string path){

	size_t find = path.find("//");
	while (find != std::string::npos){
		path.erase(find, 2);
		path.insert(find, "/");
		find = path.find("//");
	}
	return path;
}

int findTarget(config::Server server, std::string path){

	// Normalize URI
	path = decodePath(path);
	if (path.empty())
		return -1; // bad request: invalind encoding e.g. /usr/include%
	path = removeDotSegments(path);
	path = removeDoubleSlash(path);
	// Find location
	for (size_t i = 0; i < server.locations.size(); i++){
		if (matchesLocation(path, server.locations[i].path)){
			return i;
		}
	}
	return server.locations.size() -1;
}

bool	getLocation(std::vector<config::Server> servers, Request const request, RequestLocPair *rlp){

	std::string host;
	std::string reqTarget;

	host = request.getHost();
	reqTarget = request.getTarget("path");
	int host_index = findHost(servers, host);
	int target_index = findTarget(servers[host_index], reqTarget);
	if (host_index == -1 || target_index == -1)
		return false;
	rlp->first = request;
	rlp->second = servers[host_index].locations[target_index];
	return true;
}

void printParsingRes(RequestLocPair reqlocP){
	Headers headers = reqlocP.first.getHeaders();
	
	std::cout << "Method: " << reqlocP.first.getMethod()
	<< "\nRequest target: " << reqlocP.first.getTarget()
	<< "\nTarget path: " << reqlocP.first.getTarget("path")
	<< "\nTarget query: " << reqlocP.first.getTarget("query")
	<< "\nHost: " << headers.get("host")
	<< "\nLocated path: " << reqlocP.second.path << std::endl;
}

int Connection::handleRead(){

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
	if (_request.headersDone() && !_routed){
		if (!getLocation(_ls.second, _request, &_RequestLocPair)){
			Response::Writer w(_fd);
			w.writeStatusLine(Response::StatusBadRequest); // 413 Request Entity Too Large error
			w.writeHeaders(Response::getDefaultHeaders(0));
			return _reactor->removeHandler(this, 0);
		}
		_routed = true;

		if (_request.checkState(ParserState::Body) && _request.getContentLen() > _RequestLocPair.second.client_max_body_size) {
			Response::Writer w(_fd);
			w.writeStatusLine(Response::StatusBadRequest); // 413 Request Entity Too Large error
			w.writeHeaders(Response::getDefaultHeaders(0));
			std::cout << "remove ..." << std::endl;
			return _reactor->removeHandler(this, 0);
		}
	}
	if (_request.done()) {
		Response::Writer w(_fd);
		if (_request.error()) {
			w.writeStatusLine(Response::StatusBadRequest);
			w.writeHeaders(Response::getDefaultHeaders(0));
		} else {
			printParsingRes(_RequestLocPair);
			//_requestHandler->handle(w, _request); //processRequest(w, _RequestLocPair);
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

Connection::~Connection() {
	delete _requestHandler;
}

// ACCEPT HANDLER
Acceptor::Acceptor(const ListenerSocket &ls) : EventHandler(ls.first), _ls(ls) {};

int Acceptor::handleRead() {

	struct sockaddr_in c_addr;
	socklen_t c_addrlen;
	std::memset(&c_addr, 0, sizeof(c_addr));
	c_addrlen = sizeof(c_addr);

	for (;;){
		int cfd = accept(_fd, (struct sockaddr *)&c_addr, &c_addrlen);
		if (cfd < 0)
			break; // accept() may return -1 with EAGAIN if no connections are pending (not an error)
		std::stringstream ss;
		ss << _fd;
		logInfo("Accepted incoming client on ", ss.str());
		int flags = fcntl(cfd, F_GETFL, 0);
		int ret = fcntl(cfd, F_SETFL, flags | O_NONBLOCK);
		if (ret == -1 || flags == -1)
			return false;
		if (!_reactor->registerHandler(new Connection(cfd, this->_ls), EPOLLIN)) // close program?
			return false;
		std::stringstream ss2;
		ss2 << cfd;
		logInfo("New client on ", ss2.str());
		
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
	if (input.compare("quit") == 0){
		std::cout << "Quit called: Shutting down ..." << std::endl;
		_reactor->stop(SET);
	}
	return true;
}

int Stdin::handleWrite(){
	return true;
}

/* int process(Data &data, int fd){

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
} */