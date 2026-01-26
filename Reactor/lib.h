/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lib.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:38:09 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/26 18:00:29 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LIB_H
# define LIB_H

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
#include "../Parser/AST.hpp"
#include "../Parser/Config.hpp"
#include "../Parser/Lexer.hpp"
#include "../Parser/Parser.hpp"
#include "../Parser/Token.hpp"
#include "../Parser/Utils.hpp"
#include "../Parser/Validator.hpp"

# define MAX 1024
# define SET 1
# define GET 0

int	error(int err);

#endif
