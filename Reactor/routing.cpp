#include "routing.hpp"
#include <cstddef>
#include <string>

size_t	findHost(const std::vector<config::Server>& servers, const std::string& host){

	for (size_t i = 0; i < servers.size(); i++){
		for (size_t j = 0; j < servers[i].server_names.size(); j++){
			if (servers[i].server_names[j] == host){
				return i;
			}
		}
	}
	return 0;
}

bool matchesLocation(const std::string& target, const std::string& location){

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

size_t findTarget(const config::Server& server, std::string path){

	// Normalize URI
	path = decodePath(path);
	if (path.empty())
		return std::string::npos; // bad request: invalind encoding e.g. /usr/include%
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

bool	findLocation(const std::vector<config::Server>& servers, const Request &request,
					config::Location& location){

	std::string host;
	std::string reqTarget;

	host = request.getHost();
	reqTarget = request.getPath();
	size_t host_index = findHost(servers, host);
	size_t target_index = findTarget(servers[host_index], reqTarget);
	if (target_index == std::string::npos)
		return false;
	location = servers[host_index].locations[target_index];
	return true;
}

bool	isCGI(const std::string& path, const std::map<std::string, std::string>& cgi) {
  size_t searchFrom = 0;
  while (searchFrom < path.size()) {
    size_t dot = path.find('.', searchFrom);
    if (dot == std::string::npos)
      return false;
    size_t extEnd = path.find('/', dot);
    if (extEnd == std::string::npos)
      extEnd = path.size();
    std::string ext = path.substr(dot, extEnd - dot);
    if (cgi.find(ext) != cgi.end())
      return true;
    searchFrom = extEnd;
  }
  return false;
}

bool	isMethodAllowed(const std::string& method, const std::set<std::string>& limit_except){
	if (limit_except.empty())
		return true;
	return limit_except.find(method) != limit_except.end();
}
