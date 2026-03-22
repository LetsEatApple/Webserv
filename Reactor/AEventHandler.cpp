/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AEventHandler.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:18:00 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/31 15:49:31 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "AEventHandler.hpp"

EventHandler::EventHandler() : _fd(-1), _reactor(){}

EventHandler::EventHandler(const int fd) : _fd(fd), _reactor() {}

void EventHandler::set_fd(const int fd){
	_fd = fd;
}

int EventHandler::get_fd() const{
	return _fd;
}

void	EventHandler::reactor(Reactor *reactor){
	_reactor = reactor;
}

EventHandler::~EventHandler(){}

bool EventHandler::checkTimeout() { return false; }
