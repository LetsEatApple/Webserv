/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reactor.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:14:10 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/27 20:41:57 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.h"
#include "server.hpp"
#include "handler.hpp"

class Reactor{ //Initiation Dispatcher
private:
	int								_epfd;
	struct epoll_event				_ev;
	struct epoll_event				_evlist[MAX];
	std::map<int, EventHandler *>	_handlers;
	Server&							_server;
	bool							_sig;
	Reactor();
public:
	Reactor(Server &server);
	int		init();
	int		registerHandler (EventHandler *handler, uint32_t event);
	int		removeHandler (EventHandler *handler, uint32_t event);
	int		modifyHandler (EventHandler *handler, uint32_t event);
	int		handleEvents();
	bool	stop (int mode);
	config::ListenerData	getServerDetails(int lfd);
	~Reactor();
};

void on_signal(int sig);