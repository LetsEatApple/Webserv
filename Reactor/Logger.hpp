#pragma once

#include "lib.hpp"

// error used on fatal error -> server shutting down
int	error(std::string err, int errno_on);

// logDebug used on client specific error -> server continues
void logDebug(std::string err, int errno_on);

void logInfo(std::string info, std::string arg);