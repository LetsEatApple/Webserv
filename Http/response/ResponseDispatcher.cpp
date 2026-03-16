#include "ResponseDispatcher.hpp"

#include <algorithm>

#include "MimeTypes.hpp"
#include "Response.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <sstream>
#include <cstdio>
#include <fstream>
#include <iomanip>

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
	if (_req.getMethod() == "POST") {
		_w.writeStatusLine(Response::StatusNoContent);
		_w.writeHeaders(Response::getDefaultHeaders(0));
		return 204;
	}
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

static bool dirEntryCmp(const ResponseDispatcher::dirEntry &entry1, const ResponseDispatcher::dirEntry &entry2)
{
	return strcmp(entry1.name.c_str(), entry2.name.c_str()) <= 0;
}

static std::string htmlEscape(std::string src)
{
	std::ostringstream os;

	for (std::string::const_iterator i = src.begin(); i != src.end(); ++i)
	{
		const std::string::value_type c = *i;

		switch (c)
		{
		case '<':
			os << "&lt;";
			break;
		case '>':
			os << "&gt;";
			break;
		case '&':
			os << "&amp;";
			break;
		case '"':
			os << "&quot;";
			break;
		case '\'':
			os << "&apos;";
			break;
		default:
			os << c;
		}
	}

	return os.str();
}

static std::string encode(std::string src)
{
	std::ostringstream os;
	os.fill('0');
	os << std::hex;

	for (std::string::const_iterator i = src.begin(); i != src.end(); ++i)
	{
		const std::string::value_type c = *i;

		if (std::isalnum(c) || c == '_' || c == '-' || c == '.' || c == '~' || c == '/')
		{
			os << c;
			continue;
		}

		os << std::uppercase;
		os << "%" << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
		os << std::nouppercase;
	}

	return os.str();
}

int ResponseDispatcher::serveAutoindex(const std::string &fsPath, const std::string &reqPath, const std::string &reqMethod)
{
	DIR *dir = opendir(fsPath.c_str());
	if (!dir)
		return sendError(Response::StatusForbidden);

	std::ostringstream html;
	html << "<html>\n<head><title>Index of " + htmlEscape(reqPath) + "</title></head>\n";
	html << "<body>\n<h1>Index of " + htmlEscape(reqPath) + "</h1><hr><pre><a href=\"../\">../</a>\n";

	std::vector<dirEntry> dir_entries;
	std::vector<dirEntry> file_entries;
	dirent* dp;
	while ((dp = readdir(dir)) != NULL) {
		std::string name = dp->d_name;
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;
		struct stat st = {};
		stat((fsPath + name).c_str(), &st);
		if (S_ISDIR(st.st_mode))
		{
			name += "/";
			dirEntry entry = {name, st.st_mtime, st.st_size};
			dir_entries.push_back(entry);
		} else
		{
			dirEntry entry = {name, st.st_mtime, st.st_size};
			file_entries.push_back(entry);
		}
	}
	closedir(dir);
	std::sort(dir_entries.begin(), dir_entries.end(), dirEntryCmp);
	std::sort(file_entries.begin(), file_entries.end(), dirEntryCmp);

	for (std::vector<dirEntry>::const_iterator it = dir_entries.begin(); it != dir_entries.end(); ++it)
	{

		if (it->name.size() > 50)
		{
			std::string shortName = it->name.substr(0, 47);
			html << "<a href=\"" + encode(it->name) + "\">" + htmlEscape(shortName)  + "..&gt;</a>";
		}
		else
		{
			html << "<a href=\"" + encode(it->name) + "\">" + htmlEscape(it->name) + "</a>";
			html << std::setw(50 - it->name.size()) << " ";
		}
		char buf[18];
		strftime(buf, 18, "%d-%b-%Y %H:%M", localtime(&it->mod_time));
		html << " " << buf;
		html << std::setw(20) << "-" << "\n";
	}

	for (std::vector<dirEntry>::const_iterator it = file_entries.begin(); it != file_entries.end(); ++it)
	{

		if (it->name.size() > 50)
		{
			std::string shortName = it->name.substr(0, 47);
			html << "<a href=\"" + encode(it->name) + "\">" + htmlEscape(shortName) + "..&gt;</a>";
		}
		else
		{
			html << "<a href=\"" + encode(it->name) + "\">" + htmlEscape(it->name) + "</a>";
			html << std::setw(50 - it->name.size()) << " ";
		}
		char buf[18];
		strftime(buf, 18, "%d-%b-%Y %H:%M", localtime(&it->mod_time));
		html << " " << buf;
		html << std::setw(20) << it->size << "\n";
	}

	html << "</pre><hr></body>\n</html>\n";
	const std::string htmlStr = html.str();
	_w.writeStatusLine(Response::StatusOk);
	Headers h = Response::getDefaultHeaders(htmlStr.size());
	h.set("Content-Type", "text/html");
	_w.writeHeaders(h);
	if (reqMethod != "HEAD")
		_w.writeBody(htmlStr.c_str(), htmlStr.size());
	return 200;
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
		std::string dirPath = fsPath;
		if (dirPath[dirPath.size() - 1] != '/')
			dirPath += "/";
		bool hasIndex = false;
		if (!_loc.index.empty()) {
			std::string indexPath = dirPath + _loc.index;
			struct stat idxSt;
			hasIndex = (stat(indexPath.c_str(), &idxSt) == 0 && S_ISREG(idxSt.st_mode));
		}
		if (fsPath[fsPath.size() - 1] != '/') {
			if (!hasIndex && !_loc.autoindex)
				return sendError(Response::StatusNotFound);
			_w.writeStatusLine(Response::StatusMovedPermanently);
			Headers h = Response::getDefaultHeaders(0);
			h.set("location", _req.getPath() + "/");
			_w.writeHeaders(h);
			return 301;
		}
		if (hasIndex) {
			fsPath = dirPath + _loc.index;
		} else if (_loc.autoindex) {
			return serveAutoindex(fsPath, _req.getPath(), _req.getMethod());
		} else {
			return sendError(Response::StatusForbidden);
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