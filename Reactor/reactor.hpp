/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reactor.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:14:10 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/14 18:32:19 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <sys/epoll.h>

#include "constants.hpp"
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
	int		init(const activeSessions &sessions);
	int		registerHandler (EventHandler *handler, uint32_t event);
	int		removeHandler (const EventHandler *handler, uint32_t event);
	int		modifyHandler (const EventHandler *handler, uint32_t event);
	void	handleEvents();
	void	checkTimeouts();
	bool	stop (int mode);
	~Reactor();
};

void on_signal(int sig);
