#pragma once

#include "Request.hpp"
#include "Config.hpp"

enum ProcessResult {
	NeedMoreData,
	RequestDone,
	RequestError,
	PayloadTooLarge,
	MethodNotAllowed
};

bool			findLocation(const std::vector<config::Server>& servers, const Request &request,
					config::Location& location);
bool			isMethodAllowed(const std::string& method, const std::set<std::string>& limit_except);
size_t			findTarget(const config::Server& server, std::string path);
std::string		removeDoubleSlash(std::string path);
std::string		removeDotSegments(std::string input);
std::string		decodePath(std::string target);
bool			unreservedChar(char c);
bool			matchesLocation(const std::string& target, const std::string& location);
size_t			findHost(const std::vector<config::Server>& servers, const std::string& host);
bool			isCGI(const std::string& path, const std::map<std::string, std::string>& cgi);
void			printParsingRes(const Request& req);
