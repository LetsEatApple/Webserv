/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lib.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:38:09 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/12 17:34:00 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LIB_HPP
# define LIB_HPP

#include <iostream>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <map>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include "Logger.hpp"
#include <ctime>

# define MAX 65536
# define SET 1
# define GET 0
# ifndef CGI_TIMEOUT_SEC
#  define CGI_TIMEOUT_SEC 120
# endif
# define MAX_HEADER_SIZE 8192
# define MAX_CGI_BUFFER 65536
# ifndef CLIENT_TIMEOUT_SEC
#  define CLIENT_TIMEOUT_SEC 600
# endif

int	error(const std::string& err, int errno_on);

#endif
