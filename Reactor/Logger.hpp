#pragma once

#include "lib.hpp"
#include "server.hpp"
#include "Headers.hpp"
#include "Request.hpp"

struct Endpoint;

// error used on fatal error -> server shutting down
int				error(std::string err, int errno_on);

// logDebug used on client specific error -> server continues
void			logDebug(std::string err, int errno_on);
void			logInfo(std::string info, std::string arg);
std::string		IP_port(sockaddr_in &c_addr);
std::string		str(uint16_t port);
void			logConnection(Endpoint listener, Endpoint client);
void			logDisconnect(Endpoint listener, Endpoint client);
void			logRequest(Endpoint listener, Endpoint Client, Request req);