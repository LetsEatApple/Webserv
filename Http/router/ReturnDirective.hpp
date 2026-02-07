#ifndef RETURNDIRECTIVE_HPP
#define RETURNDIRECTIVE_HPP

#include "Response.hpp"
#include "Config.hpp"
#include <vector>

class Request;

bool serveReturn(Response::Writer& w, const Request& req, const std::vector<config::Server>& servers);

#endif
