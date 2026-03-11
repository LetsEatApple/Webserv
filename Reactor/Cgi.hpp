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
  CgiProcess(Connection& conn);
  ~CgiProcess();

  bool start();
  void stop();
  int onWritable();
  int onReadable();
  void onHandlerClosed(CgiFdHandler::Kind kind);

  bool isRunning() const;
  std::time_t startTime() const;

private:
  Connection& _conn;
  pid_t _pid;
  int _inFd;
  int _outFd;
  bool _inClosed;
  std::string _inBuffer;
  std::string _outBuffer;
  CgiFdHandler* _inHandler;
  CgiFdHandler* _outHandler;
  std::time_t _startTime;

  bool closeInputIfDone();
  bool buildHttpResponse();
};
