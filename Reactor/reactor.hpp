/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reactor.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:14:10 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/31 16:42:59 by lhagemos         ###   ########.fr       */
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
	bool							_sig;
public:
	Reactor();
	int		init(const serverListeners &listeners);
	int		registerHandler (EventHandler *handler, uint32_t event);
	int		removeHandler (EventHandler *handler, uint32_t event);
	int		modifyHandler (EventHandler *handler, uint32_t event);
	int		handleEvents();
	bool	stop (int mode);
	~Reactor();
};

void on_signal(int sig);