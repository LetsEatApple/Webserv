#pragma once

#include "lib.hpp"
#include "server.hpp"
#include "Headers.hpp"
#include "Request.hpp"

struct Endpoint;

// error used on fatal error -> server shutting down
int				error(const std::string& err, int errno_on);

// logDebug used on client specific error -> server continues
void			logDebug(const std::string& err, int errno_on);
void			logInfo(const std::string& info, const std::string& arg);
std::string		IP_port(sockaddr_in &c_addr);
std::string		str(uint16_t port);
void			logConnection(const Endpoint& listener, const Endpoint& client);
void			logDisconnect(const Endpoint& listener, const Endpoint& client);
void			logRequest(const Endpoint& listener, const Endpoint& client, const Request& req);
