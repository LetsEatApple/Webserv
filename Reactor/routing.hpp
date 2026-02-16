#pragma once

#include "lib.hpp"
#include "Request.hpp"

typedef std::pair<Request, config::Location>	RequestLocPair;


bool			findLocation(std::vector<config::Server> servers, Request &request);
int				findTarget(config::Server server, std::string path);
std::string		removeDoubleSlash(std::string path);
std::string		removeDotSegments(std::string input);
std::string		decodePath(std::string target);
bool			unreservedChar(char c);
bool			matchesLocation(std::string target, std::string location);
int				findHost(std::vector<config::Server> servers, std::string host);
void			printParsingRes(Request req);