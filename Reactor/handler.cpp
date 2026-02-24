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
#include "Headers.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "routing.hpp"
#include "reactor.hpp"
#include "ResponseDispatcher.hpp"
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <vector>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

CgiFdHandler::CgiFdHandler(int fd, Connection* owner, Kind kind) : EventHandler(fd), _owner(owner),
  _kind(kind) {}

CgiFdHandler::~CgiFdHandler() {
  if (_owner)
    _owner->onCgiHandlerClosed(_kind);
}
int CgiFdHandler::handleRead() { return (_kind == CGI_OUT) ? _owner->onCgiReadable() : true; }
int CgiFdHandler::handleWrite() { return (_kind == CGI_IN) ? _owner->onCgiWritable() : true; }

//	CONNECTION HANDLER
Connection::Connection(const SessionInfo &session) : EventHandler(session.client.socket), _session(session),
                                                     _location(),
                                                     _routed(false), _closeAfterResponse(false),
_state(StateReadingHeaders), _isCgi(false), _cgiPid(-1), _cgiInFd(-1), _cgiOutFd(-1), _cgiInClosed(false), _cgiInBuffer(), _cgiOutBuffer(),
_cgiInHandler(NULL), _cgiOutHandler(NULL), _cgiStartTime(0) {
}

Connection::~Connection() {
  _state = StateClosing;
  stopCgi(true);
}

static bool makeNonBlocking(int fd) {
  int fl = fcntl(fd, F_GETFL, 0);
  return (fl >= 0 && fcntl(fd, F_SETFL, fl | O_NONBLOCK) == 0);
}

static std::string dirNameOf(const std::string& p) {
  size_t pos = p.find_last_of('/');
  if (pos == std::string::npos) return ".";
  if (pos == 0) return "/";
  return p.substr(0, pos);
}

static void addEnv(std::vector<std::string>& env, const std::string& k, const std::string& v) {
  env.push_back(k + "=" + v);
}

struct AddHttpEnv {
  std::vector<std::string>& env;
  AddHttpEnv(std::vector<std::string>& e) : env(e) {}
  void operator()(const std::string& name, const std::string& value) const {
    if (name == "content-length" || name == "content-type")
      return;
    std::string envName = "HTTP_";
    for (size_t i = 0; i < name.size(); ++i) {
      char c = name[i];
      if (c == '-') envName += '_';
      else if (c >= 'a' && c <= 'z') envName += static_cast<char>(c - 32);
      else envName += c;
    }
    env.push_back(envName + "=" + value);
  }
};

bool Connection::startCgi() {
  const std::string reqPath = _request.getPath(); // /perl/perl.cgi 

  std::string script;
  std::string pathInfo;
  std::string ext;
  {
    size_t searchFrom = 0;
    bool found = false;
    while (searchFrom < reqPath.size()) {
      size_t dot = reqPath.find('.', searchFrom);
      if (dot == std::string::npos) break;
      size_t extEnd = reqPath.find('/', dot);
      if (extEnd == std::string::npos) extEnd = reqPath.size();
      std::string candidate = reqPath.substr(dot, extEnd - dot);
      if (_location.cgi.find(candidate) != _location.cgi.end()) {
        ext = candidate;
        size_t prev_slash = reqPath.rfind('/', dot);
        if (prev_slash == std::string::npos)
          script = reqPath.substr(0, extEnd);
        else
          script = reqPath.substr(prev_slash, extEnd);
        pathInfo = reqPath;//reqPath.substr(extEnd);
        found = true;
        break;
      }
      searchFrom = extEnd;
    }
    if (!found) return false;
  }

  std::map<std::string, std::string>::const_iterator it = _location.cgi.find(ext);
  if (it == _location.cgi.end()) return false;
  const std::string  interpreter = it->second;

  int inPipe[2] = {-1, -1};
  int outPipe[2] = {-1, -1};
  if (pipe(inPipe) != 0 || pipe(outPipe) != 0) {
    if (inPipe[0] != -1) close(inPipe[0]);
    if (inPipe[1] != -1) close(inPipe[1]);
    if (outPipe[0] != -1) close(outPipe[0]);
    if (outPipe[1] != -1) close(outPipe[1]);
    return false;
  }
  if (!makeNonBlocking(inPipe[1]) || !makeNonBlocking(outPipe[0])) {
    close(inPipe[0]); close(inPipe[1]);
    close(outPipe[0]); close(outPipe[1]);
    return false;
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(inPipe[0]); close(inPipe[1]);
    close(outPipe[0]); close(outPipe[1]);
    return false;
  }

  const std::string scriptFileName = _location.alias + script;
  const std::string scriptName = script;
  const std::string queryString = _request.getQuery();
  const std::string requestMethod = _request.getMethod();
  const std::string protocolVersion = _request.getHttpVersion().empty() ? "1.1" : _request.getHttpVersion();
  const std::string serverProtocol = "HTTP/" + protocolVersion;
  const std::string documentRoot = _location.alias;
  const std::string pathTranslated = pathInfo.empty() ? "" : _location.alias + pathInfo;

  std::ostringstream cl;
  cl << _request.getBody().size();

  if (pid == 0) {
    dup2(inPipe[0], STDIN_FILENO);
    dup2(outPipe[1], STDOUT_FILENO);
    dup2(outPipe[1], STDERR_FILENO);

    close(inPipe[0]); close(inPipe[1]);
    close(outPipe[0]); close(outPipe[1]);

    const std::string cwd = dirNameOf(scriptFileName);
    if (chdir(cwd.c_str()) != 0) _exit(126);

    std::vector<std::string> envStore;
    addEnv(envStore, "GATEWAY_INTERFACE", "CGI/1.1");
    addEnv(envStore, "SERVER_PROTOCOL", serverProtocol);
    addEnv(envStore, "REQUEST_METHOD", requestMethod);
    addEnv(envStore, "SCRIPT_NAME", scriptName);
    addEnv(envStore, "SCRIPT_FILENAME", scriptFileName);
    addEnv(envStore, "QUERY_STRING", queryString);
    if (!pathInfo.empty()) {
      addEnv(envStore, "PATH_INFO", pathInfo);
      addEnv(envStore, "PATH_TRANSLATED", pathTranslated);
    }
    addEnv(envStore, "DOCUMENT_ROOT", documentRoot);
    addEnv(envStore, "REQUEST_URI", _request.getTarget());
    addEnv(envStore, "SERVER_NAME", _request.getHost());
    addEnv(envStore, "REDIRECT_STATUS", "200");
    if (requestMethod == "POST") {
      addEnv(envStore, "CONTENT_LENGTH", cl.str());

      std::string contentType = _request.getHeaders().get("content-type");
      if (!contentType.empty())
        addEnv(envStore, "CONTENT_TYPE", contentType);
    }

    const Headers& hdrs = _request.getHeaders();
    hdrs.forEach(AddHttpEnv(envStore));

    std::vector<char*> envp;
    envp.reserve(envStore.size() + 1);
    for (size_t i = 0; i < envStore.size(); ++i)
      envp.push_back(const_cast<char*>(envStore[i].c_str()));
    envp.push_back(NULL);

    char* argv[3];
    argv[0] = const_cast<char*>(interpreter.c_str());
    argv[1] = const_cast<char*>(scriptFileName.c_str());
    argv[2] = NULL;

    execve(argv[0], argv, &envp[0]);
    _exit(127);
  }

  close(inPipe[0]);
  close(outPipe[1]);

  _cgiPid = pid;
  _cgiInFd = inPipe[1];
  _cgiOutFd = outPipe[0];
  _isCgi = true;
  _cgiInClosed = false;
  _cgiInBuffer = _request.getBody();
  _cgiOutBuffer.clear();

  _cgiInHandler = new CgiFdHandler(_cgiInFd, this, CgiFdHandler::CGI_IN);
  _cgiOutHandler = new CgiFdHandler(_cgiOutFd, this, CgiFdHandler::CGI_OUT);

  if (!_reactor->registerHandler(_cgiInHandler, EPOLLOUT)) {
    stopCgi(true);
    return false;
  }
  if (!_reactor->registerHandler(_cgiOutHandler, EPOLLIN)) {
    stopCgi(true);
    return false;
  }
  
  if (_cgiInBuffer.empty()) {
    closeCgiInputIfDone(); // sends EOF to CGI stdin for GET
}

  _state = StateCgiRunning;
  _cgiStartTime = time(NULL);
  return true;
}

void Connection::stopCgi(bool killChild) {
    if (_cgiInHandler) {
        _reactor->removeHandler(_cgiInHandler, 0);
    }
    if (_cgiOutHandler) {
        _reactor->removeHandler(_cgiOutHandler, 0);
    }
    if (_cgiInFd >= 0) {
        close(_cgiInFd);
        _cgiInFd = -1;
    }
    if (_cgiOutFd >= 0) {
        close(_cgiOutFd);
        _cgiOutFd = -1;
    }
    if (_cgiPid > 0) {
      int status;
      if (waitpid(_cgiPid, &status, WNOHANG) == 0) {
          if (killChild) {
            kill(_cgiPid, SIGKILL);
            waitpid(_cgiPid, &status, 0);
          }
      }
    _cgiPid = -1;
    }

    _isCgi = false;
    _cgiInClosed = true;
    _cgiInBuffer.clear();
    _cgiOutBuffer.clear();
}

bool Connection::closeCgiInputIfDone() {
    if (_cgiInClosed) return true;
    if (!_cgiInBuffer.empty()) return true;

    if (_cgiInHandler) {
        _reactor->removeHandler(_cgiInHandler, 0);
    }
    if (_cgiInFd >= 0) {
        close(_cgiInFd);
        _cgiInFd = -1;
    }
    _cgiInClosed = true;
    return true;
}

void Connection::onCgiHandlerClosed(CgiFdHandler::Kind kind) {
  if (_state == StateClosing)
    return;
  if (kind == CgiFdHandler::CGI_IN) {
    _cgiInHandler = NULL;
    _cgiInFd = -1;
  } else {
    _cgiOutHandler = NULL;
    _cgiOutFd = -1;
  }
}

  void pr(const std::string& f, const std::string& s) {
    std::cout << f << ":  " << s << std::endl;
  }
bool Connection::buildHttpResponseFromCgi() {
    size_t sep = _cgiOutBuffer.find("\r\n\r\n");
    size_t sepLen = 4;
    //if (sep == std::string::npos) {
    //    sep = _cgiOutBuffer.find("\n\n");
    //    sepLen = 2;
    //}

    if (sep == std::string::npos)
      return false;

    // No CGI headers: treat entire output as body TODO: should be an error?
    //if (sep == std::string::npos) {
    //    Response::Writer w(_writeBuffer);
    //    w.writeStatusLine(Response::StatusOk);
    //    Headers h = Response::getDefaultHeaders(_cgiOutBuffer.size());
    //    h.set("Connection", "close");
    //    w.writeHeaders(h);
    //    w.writeBody(_cgiOutBuffer.c_str(), _cgiOutBuffer.size());
    //    return true;
    //}
    
    std::string rawHeaders = _cgiOutBuffer.substr(0, sep);
    std::string body = _cgiOutBuffer.substr(sep + sepLen);

    //std::cerr << "TOTAL CGI OUT SIZE: " << _cgiOutBuffer.size() << std::endl;
    //std::cerr << "SEP POSITION: " << sep << std::endl;
    //std::cerr << "BODY SIZE: " << body.size() << std::endl;
    
    int status = 200;
    Headers cgiHeaders;
    //bool hasContentLength = false;
    bool hasContentType = false;

    std::istringstream hs(rawHeaders);
    std::string line;
    while (std::getline(hs, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string name = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
            value.erase(0, 1);

        if (name == "Status") {
            status = std::atoi(value.c_str());
            if (status <= 0) status = 200;
            continue;
        }
        if (name == "Content-Length" || name == "content-length")
            //hasContentLength = true;
            continue;
        if (name == "Content-Type")
            hasContentType = true;

        cgiHeaders.set(name, value);
    }

    Response::StatusCode code = Response::intToStatusCode(status);
    Response::Writer w(_writeBuffer);
    w.writeStatusLine(code);

    std::ostringstream oss;
    oss << body.size();
    cgiHeaders.set("Content-Length", oss.str());
    if (!hasContentType)
        cgiHeaders.set("Content-Type", "text/html");

    //cgiHeaders.forEach(pr);

    w.writeHeaders(cgiHeaders);

    w.writeBody(body.c_str(), body.size());
    //std::cerr << "WRITE BUFFER SIZE: " << _writeBuffer.size() << std::endl;
    return true;
}

int Connection::onCgiWritable() {
    if (!_isCgi || _cgiInFd < 0) return true;

    if (!_cgiInBuffer.empty()) {
        ssize_t n = write(_cgiInFd, _cgiInBuffer.c_str(), _cgiInBuffer.size());
        if (n > 0)
            _cgiInBuffer.erase(0, static_cast<size_t>(n));
        else
            return true;
    }

    return closeCgiInputIfDone();
}

int Connection::onCgiReadable() {
    if (!_isCgi || _cgiOutFd < 0) return true;

    char buf[MAX];
    ssize_t n = read(_cgiOutFd, buf, sizeof(buf));
    if (n > 0) {
        _cgiOutBuffer.append(buf, static_cast<size_t>(n));
        return true;
    }
    if (n == 0) {
        if (!buildHttpResponseFromCgi()) {
            _closeAfterResponse = true;
            Response::Writer w(_writeBuffer);
            ResponseDispatcher(w, _request, _location).sendError(Response::StatusBadGateway);
        }
        stopCgi(false);
        _state = StateSendingResponse;
        return _reactor->modifyHandler(this, EPOLLOUT);
    }
    return true;
}

bool Connection::checkTimeout() {
  if (_state != StateCgiRunning || _cgiStartTime == 0)
    return false;
  if (time(NULL) - _cgiStartTime < CGI_TIMEOUT_SEC)
    return false;
  logInfo("CGI timeout after ", str(CGI_TIMEOUT_SEC).append("s - killing process"));
  _closeAfterResponse = true;
  Response::Writer w(_writeBuffer);
  ResponseDispatcher(w, _request, _location).sendError(Response::StatusBadGateway);
  stopCgi(true);
  _state = StateSendingResponse;
  _reactor->modifyHandler(this, EPOLLOUT);
  return false;
}

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
	if (_request.done()) {
    if (_routed && !isMethodAllowed(_request.getMethod(), _location.limit_except))
			return MethodNotAllowed;
    if (!_isCgi && _routed && isCGI(_request.getPath(), _location.cgi)) {
      if (!startCgi())
        return RequestError;
      return NeedMoreData;
    }
    if (_isCgi)
      return NeedMoreData;
		return RequestDone;
  }

	return NeedMoreData;
}

int Connection::handleRead(){

  if (_state == StateCgiRunning) {
    char buf[MAX];
    ssize_t n = recv(_fd, buf, MAX - 1, MSG_DONTWAIT);
    if (n == 0) {
      stopCgi(true);
      logDisconnect(_session.listener, _session.client);
      return (_reactor->removeHandler(this, 0));
    }
    if (n > 0)
      _readBuffer.append(buf, n);
    return true;
  }

	char buf[MAX];
	ssize_t n = recv(_fd, buf, MAX - 1, MSG_DONTWAIT);
	if (n == 0){ // client disconnected
    stopCgi(true);
		logDisconnect(_session.listener, _session.client);
		return (_reactor->removeHandler(this, 0));
	}
	if (n > 0)
		_readBuffer.append(buf, n);
	else if (_readBuffer.empty()) // Error occured and buffer empty, epoll decides what to do
		return false;
	const ProcessResult result = processRequest();

	switch (result){
		case RequestError: {
			_closeAfterResponse = true;
			Response::Writer w(_writeBuffer);
			ResponseDispatcher(w, _request, _location).sendError(Response::StatusBadRequest);
			_state = StateSendingResponse;
      return _reactor->modifyHandler(this, EPOLLOUT);
		}
		case PayloadTooLarge: {
			_closeAfterResponse = true;
			Response::Writer w(_writeBuffer);
			ResponseDispatcher(w, _request, _location).sendError(Response::StatusPayloadTooLarge);
			_state = StateSendingResponse;
			return _reactor->modifyHandler(this, EPOLLOUT);
		}
		case RequestDone: {
			Response::Writer w(_writeBuffer);
			if (_request.error()) {
				_closeAfterResponse = true;
				ResponseDispatcher(w, _request, _location).sendError(Response::StatusBadRequest);
			} else {
				logRequest(_session.listener, _session.client, _request, _location.path);
				ResponseDispatcher(w, _request, _location).serve();
			}
			_state = StateSendingResponse;
			return _reactor->modifyHandler(this, EPOLLOUT);
		}
		case MethodNotAllowed: {
			Response::Writer w(_writeBuffer);
			ResponseDispatcher(w, _request, _location).sendError(Response::StatusMethodNotAllowed);
			_state = StateSendingResponse;
			return _reactor->modifyHandler(this, EPOLLOUT);
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

	if (_writeBuffer.empty()) {
		if (_closeAfterResponse) {
			stopCgi(true);
      logDisconnect(_session.listener, _session.client);
			return _reactor->removeHandler(this, 0);
		}
		_request.reset();
		_routed = false;
    _state = StateReadingHeaders;
		if (!_readBuffer.empty())
			return handleRead();
		return (_reactor->modifyHandler(this, EPOLLIN));
	}

	const ssize_t sent = send(_fd, _writeBuffer.c_str(), _writeBuffer.length(), MSG_DONTWAIT);
	if (sent > 0)
		_writeBuffer.erase(0, sent);
	if (_writeBuffer.empty()) {
		if (_closeAfterResponse) {
      stopCgi(true);
			logDisconnect(_session.listener, _session.client);
			return _reactor->removeHandler(this, 0);
		}
		_request.reset();
		_routed = false;
    _state = StateReadingHeaders;
		if (!_readBuffer.empty())
			return handleRead();
		return (_reactor->modifyHandler(this, EPOLLIN));
	}
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
		Connection* conn = new Connection(this->_session);
		if (!_reactor->registerHandler(conn, EPOLLIN)) {
			close(cfd);
			delete conn;
			return false;
		}
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
