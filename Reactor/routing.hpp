#pragma once

#include "lib.hpp"
#include "Request.hpp"

typedef std::pair<Request, config::Location>	RequestLocPair;


bool			findLocation(const std::vector<config::Server>& servers, Request &request);
size_t			findTarget(const config::Server& server, std::string path);
std::string		removeDoubleSlash(std::string path);
std::string		removeDotSegments(std::string input);
std::string		decodePath(std::string target);
bool			unreservedChar(char c);
bool			matchesLocation(const std::string& target, const std::string& location);
size_t			findHost(const std::vector<config::Server>& servers, const std::string& host);
void			printParsingRes(const Request& req);
