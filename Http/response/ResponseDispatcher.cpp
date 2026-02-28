#include "ResponseDispatcher.hpp"
#include "MimeTypes.hpp"
#include "Response.hpp"
#include <sys/stat.h>
#include <cstdio>
#include <fstream>

static bool hasPathTraversal(const std::string& path) {
	if (path.find("/..") != std::string::npos)
		return true;
	if (path.compare(0, 3, "../") == 0)
		return true;
	return false;
}

static std::string resolvePath(const config::Location& loc, const std::string& reqPath) {
	std::string rel = reqPath.substr(loc.path.size());
	if (hasPathTraversal(rel))
		return "";
	return loc.alias + "/" + rel;
}

static bool isRedirect(int code) {
	return code == 301 || code == 302 || code == 307 || code == 308;
}

ResponseDispatcher::ResponseDispatcher(Response::Writer& w, const Request& req, const config::Location& loc)
	: _w(w), _req(req), _loc(loc) {}

int ResponseDispatcher::sendError(Response::StatusCode code) {
	std::string body;
	std::string contentType = "text/html";
	std::map<int, std::string>::const_iterator it = _loc.error_page.find(static_cast<int>(code));
	if (it != _loc.error_page.end()) {
		std::ifstream file(it->second.c_str(), std::ios::binary);
		if (file) {
			body.assign(std::istreambuf_iterator<char>(file),
						std::istreambuf_iterator<char>());
			contentType = getMimeType(it->second);
		}
	}
	if (body.empty())
		body = Response::getDefaultErrorPage(code);
	_w.writeStatusLine(code);
	Headers h = Response::getDefaultHeaders(body.size());
	h.replace("Content-Type", contentType);
	_w.writeHeaders(h);
	if (_req.getMethod() != "HEAD")
		_w.writeBody(body.c_str(), body.size());
	return static_cast<int>(code);
}

int ResponseDispatcher::serve() {
	if (!_loc.ret.empty())
		return serveReturn();
	if (_req.getMethod() == "DELETE")
		return serveDelete();
	return serveFile();
}

int ResponseDispatcher::serveReturn() {
	std::map<int, std::string>::const_iterator it = _loc.ret.begin();
	int code = it->first;
	const std::string& value = it->second;

	Response::StatusCode status = Response::intToStatusCode(code);
	_w.writeStatusLine(status);

	if (isRedirect(code)) {
		Headers h = Response::getDefaultHeaders(0);
		h.set("location", value);
		_w.writeHeaders(h);
	} else {
		_w.writeHeaders(Response::getDefaultHeaders(value.size()));
		if (_req.getMethod() != "HEAD")
			_w.writeBody(value.c_str(), value.size());
	}
	return code;
}

int ResponseDispatcher::serveFile() {
	// 1. resolve filesystem path from alias + request path
    std::string fsPath = resolvePath(_loc, _req.getPath());
	struct stat st;
    // 2. stat() the path (not found -> 404)
	if (stat(fsPath.c_str(), &st) < 0)
		return sendError(Response::StatusNotFound);
	// 3. directory -> redirect 301 if missing trailing /, else try index, else autoindex, else 403
	if (S_ISDIR(st.st_mode)) {
		if (fsPath[fsPath.size() - 1] != '/') {
			_w.writeStatusLine(Response::StatusMovedPermanently);
			Headers h = Response::getDefaultHeaders(0);
			h.set("location", _req.getPath() + "/");
			_w.writeHeaders(h);
			return 301;
		}
		if (!_loc.index.empty()) {
			std::string indexPath = fsPath + _loc.index;
			if (stat(indexPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
				fsPath = indexPath;
			} else if (_loc.autoindex) {
				return sendError(Response::StatusForbidden);
			} else {
				return sendError(Response::StatusNotFound);
			}
		} else if (_loc.autoindex) {
			return sendError(Response::StatusForbidden);
		} else {
			return sendError(Response::StatusNotFound);
		}
	}
	// 4. regular file -> read, write status 200 + Content-Type from getMimeType()
	std::ifstream file(fsPath.c_str(), std::ios::binary);
	if (!file)
		return sendError(Response::StatusInternalServerError);
	std::string body((std::istreambuf_iterator<char>(file)),
					  std::istreambuf_iterator<char>());

	_w.writeStatusLine(Response::StatusOk);
	Headers h = Response::getDefaultHeaders(body.size());
	h.replace("Content-Type", getMimeType(fsPath));
	_w.writeHeaders(h);
	// 5. HEAD -> same headers, skip body
	if (_req.getMethod() != "HEAD")
		_w.writeBody(body.c_str(), body.size());
	// 6. return the status code sent
	return 200;
}

int ResponseDispatcher::serveDelete() {
	std::string fsPath = resolvePath(_loc, _req.getPath());
	struct stat st;
	if (stat(fsPath.c_str(), &st) < 0)
		return sendError(Response::StatusNotFound);
	if (std::remove(fsPath.c_str()) != 0)
		return sendError(Response::StatusForbidden);
	_w.writeStatusLine(Response::StatusNoContent);
	_w.writeHeaders(Response::getDefaultHeaders(0));
	return 204;
}

