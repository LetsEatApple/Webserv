#pragma once

#include "AEventHandler.hpp"
#include <string>
#include <ctime>

class Connection;
class CgiProcess;

class CgiFdHandler : public EventHandler {
public:
	enum Kind { CGI_IN, CGI_OUT };
	CgiFdHandler(int fd, CgiProcess *owner, Kind kind);
	~CgiFdHandler();
	int handleRead();
	int handleWrite();
private:
	CgiProcess *_owner;
	Kind _kind;
};

class CgiProcess {
public:
	explicit CgiProcess(Connection& conn);
	~CgiProcess();

	bool start();
	void stop();
	int onWritable();
	int onReadable();
	void onHandlerClosed(CgiFdHandler::Kind kind);

	bool isRunning() const;
	std::time_t startTime() const;
	bool headersSent() const;
	void resumeRead();
	void feedBody(std::string& body);
	void notifyBodyComplete();

private:
	Connection& _conn;
	pid_t _pid;
	int _inFd;
	int _outFd;
	bool _inClosed;
	bool _headersSent;
	bool _outPaused;
	bool _bodyComplete;
	bool _clientPaused;
	std::string _inBuffer;
	size_t _inOffset;
	std::string _outBuffer;
	CgiFdHandler* _inHandler;
	CgiFdHandler* _outHandler;
	std::time_t _startTime;

	void closeInputIfDone();
	bool trySendCgiHeaders();
	void pauseRead();
	void pauseClientRead();
	void resumeClientRead();
};
