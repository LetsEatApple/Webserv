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
#include "handler.hpp"

EventHandler::EventHandler() : _fd(-1){};

EventHandler::EventHandler(int fd) : _fd(fd) {};

void EventHandler::set_fd(int fd){

	_fd = fd;
};

int EventHandler::get_fd(){

	return _fd;
};

void	EventHandler::reactor(Reactor *reactor){

	_reactor = reactor;
}

EventHandler::~EventHandler(){}

/* int EventHandler::fd(int mode, int fd){

	if (mode == SET)
		_fd = fd;

	return _fd;
} */

bool EventHandler::checkTimeout() { return false; }
