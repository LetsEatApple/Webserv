#include "Cgi.hpp"
#include "handler.hpp"
#include "reactor.hpp"
#include "Response.hpp"
#include "ResponseDispatcher.hpp"

#include <algorithm>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>

CgiFdHandler::CgiFdHandler(const int fd, CgiProcess* owner, const Kind kind)
  : EventHandler(fd), _owner(owner), _kind(kind) {}

CgiFdHandler::~CgiFdHandler() {
  if (_owner)
    _owner->onHandlerClosed(_kind);
}

int CgiFdHandler::handleRead() {
  return (_kind == CGI_OUT) ? _owner->onReadable() : true;
}

int CgiFdHandler::handleWrite() {
  return (_kind == CGI_IN) ? _owner->onWritable() : true;
}

static std::string toLower(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 static_cast<int(*)(int)>(std::tolower));
  return result;
}

static std::string dirNameOf(const std::string& p) {
  size_t pos = p.find_last_of('/');
  if (pos == std::string::npos) return ".";
  if (pos == 0) return "/";
  return p.substr(0, pos);
}

static void addEnv(std::vector<std::string>& env,
                   const std::string& k, const std::string& v) {
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

CgiProcess::CgiProcess(Connection& conn)
  : _conn(conn), _pid(-1), _inFd(-1), _outFd(-1), _inClosed(false),
    _inHandler(NULL), _outHandler(NULL), _startTime(0) {}

CgiProcess::~CgiProcess() {
  stop();
}

bool CgiProcess::isRunning() const { return _pid > 0; }
std::time_t CgiProcess::startTime() const { return _startTime; }

bool CgiProcess::start() {
  const Request& req = _conn.request();
  const config::Location& loc = _conn.location();
  const std::string reqPath = req.getPath();

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
      if (loc.cgi.find(candidate) != loc.cgi.end()) {
        ext = candidate;
        size_t prev_slash = reqPath.rfind('/', dot);
        if (prev_slash == std::string::npos)
          script = reqPath.substr(0, extEnd);
        else
          script = reqPath.substr(prev_slash, extEnd);
        pathInfo = reqPath;
        found = true;
        break;
      }
      searchFrom = extEnd;
    }
    if (!found) return false;
  }

  std::map<std::string, std::string>::const_iterator it = loc.cgi.find(ext);
  if (it == loc.cgi.end()) return false;
  const std::string interpreter = it->second;

  int inPipe[2] = {-1, -1};
  int outPipe[2] = {-1, -1};
  if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, inPipe) != 0
      || socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, outPipe) != 0) {
    if (inPipe[0] != -1) close(inPipe[0]);
    if (inPipe[1] != -1) close(inPipe[1]);
    if (outPipe[0] != -1) close(outPipe[0]);
    if (outPipe[1] != -1) close(outPipe[1]);
    return false;
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(inPipe[0]); close(inPipe[1]);
    close(outPipe[0]); close(outPipe[1]);
    return false;
  }

  const std::string scriptFileName = loc.alias + script;
  const std::string scriptName = script;
  const std::string queryString = req.getQuery();
  const std::string requestMethod = req.getMethod();
  const std::string protocolVersion =
    req.getHttpVersion().empty() ? "1.1" : req.getHttpVersion();
  const std::string serverProtocol = "HTTP/" + protocolVersion;
  const std::string documentRoot = loc.alias;
  const std::string pathTranslated =
    pathInfo.empty() ? "" : loc.alias + pathInfo;

  std::ostringstream cl;
  cl << req.getBody().size();

  if (pid == 0) {
    dup2(inPipe[0], STDIN_FILENO);
    dup2(outPipe[1], STDOUT_FILENO);
    dup2(outPipe[1], STDERR_FILENO);

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
    addEnv(envStore, "REQUEST_URI", req.getTarget());
    addEnv(envStore, "SERVER_NAME", req.getHost());
    //TODO: addEnv(envStore, "SERVER_PORT");
    addEnv(envStore, "REDIRECT_STATUS", "200");
    if (requestMethod == "POST") {
      addEnv(envStore, "CONTENT_LENGTH", cl.str());
      std::string contentType = req.getHeaders().get("content-type");
      if (!contentType.empty())
        addEnv(envStore, "CONTENT_TYPE", contentType);
    }

    const Headers& hdrs = req.getHeaders();
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

  _pid = pid;
  _inFd = inPipe[1];
  _outFd = outPipe[0];
  _inClosed = false;
  _inBuffer = req.getBody();
  _outBuffer.clear();

  _inHandler = new CgiFdHandler(_inFd, this, CgiFdHandler::CGI_IN);
  _outHandler = new CgiFdHandler(_outFd, this, CgiFdHandler::CGI_OUT);

  if (!_conn.reactor()->registerHandler(_inHandler, EPOLLOUT)) {
    stop();
    return false;
  }
  if (!_conn.reactor()->registerHandler(_outHandler, EPOLLIN)) {
    stop();
    return false;
  }

  if (_inBuffer.empty())
    closeInputIfDone();

  _startTime = std::time(NULL);
  return true;
}

void CgiProcess::stop() {
  if (_inHandler) {
    _conn.reactor()->removeHandler(_inHandler, 0);
  }
  if (_outHandler) {
    _conn.reactor()->removeHandler(_outHandler, 0);
  }
  if (_inFd >= 0) {
    close(_inFd);
    _inFd = -1;
  }
  if (_outFd >= 0) {
    close(_outFd);
    _outFd = -1;
  }
  if (_pid > 0) {
    int status;
    if (waitpid(_pid, &status, WNOHANG) == 0) {
      kill(_pid, SIGKILL);
      waitpid(_pid, &status, 0);
    }
    _pid = -1;
  }
  _inClosed = true;
  _inBuffer.clear();
  _outBuffer.clear();
}

bool CgiProcess::closeInputIfDone() {
  if (_inClosed) return true;
  if (!_inBuffer.empty()) return true;

  if (_inHandler) {
    _conn.reactor()->removeHandler(_inHandler, 0);
  }
  if (_inFd >= 0) {
    close(_inFd);
    _inFd = -1;
  }
  _inClosed = true;
  return true;
}

void CgiProcess::onHandlerClosed(CgiFdHandler::Kind kind) {
  if (_conn.state() == StateClosing)
    return;
  if (kind == CgiFdHandler::CGI_IN) {
    _inHandler = NULL;
    _inFd = -1;
  } else {
    _outHandler = NULL;
    _outFd = -1;
  }
}

bool CgiProcess::buildHttpResponse() {
  size_t sep = _outBuffer.find("\r\n\r\n");
  size_t sepLen = 4;

  if (sep == std::string::npos) {
    sep = _outBuffer.find("\n\n");
    sepLen = 2;
  }

  if (sep == std::string::npos)
    return false;

  std::string rawHeaders = _outBuffer.substr(0, sep);
  std::string body = _outBuffer.substr(sep + sepLen);

  int status = 200;
  Headers cgiHeaders;
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

    if (toLower(name) == "status") {
      char *end;
      const long s = std::strtol(value.c_str(), &end, 10);
      if (s > 0)
        status = static_cast<int>(s);
      continue;
    }
    if (toLower(name) == "content-length")
      continue;
    if (toLower(name) == "content-type")
      hasContentType = true;

    if (toLower(name) == "set-cookie") {
      cgiHeaders.add(name, value);
      continue;
    }

    cgiHeaders.set(name, value);
  }

  Response::StatusCode code = Response::intToStatusCode(status);
  Response::Writer w(_conn.writeBuffer());
  w.writeStatusLine(code);

  std::ostringstream oss;
  oss << body.size();
  cgiHeaders.set("Content-Length", oss.str());
  if (!hasContentType)
    cgiHeaders.set("Content-Type", "text/html");

  w.writeHeaders(cgiHeaders);
  w.writeBody(body.c_str(), body.size());
  return true;
}

int CgiProcess::onWritable() {
  if (_inFd < 0) return true;

  if (!_inBuffer.empty()) {
    ssize_t n = send(_inFd, _inBuffer.c_str(), _inBuffer.size(), MSG_DONTWAIT);
    if (n > 0)
      _inBuffer.erase(0, static_cast<size_t>(n));
    else
      return true;
  }

  return closeInputIfDone();
}

int CgiProcess::onReadable() {
  if (_outFd < 0) return true;

  char buf[MAX];
  for (;;) {
    ssize_t n = recv(_outFd, buf, sizeof(buf), MSG_DONTWAIT);
    if (n > 0) {
      _outBuffer.append(buf, static_cast<size_t>(n));
      continue;
    }
    if (n == 0) {
      if (!buildHttpResponse()) {
        _conn.setCloseAfterResponse(true);
        Response::Writer w(_conn.writeBuffer());
        ResponseDispatcher(w, _conn.request(), _conn.location())
          .sendError(Response::StatusBadGateway);
      }
      stop();
      _conn.setState(StateSendingResponse);
      return _conn.reactor()->modifyHandler(&_conn, EPOLLOUT);
    }
    break;
  }
  return true;
}
