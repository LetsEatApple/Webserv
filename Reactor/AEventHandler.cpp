/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AEventHandler.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:18:00 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/23 13:51:12 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "AEventHandler.hpp"

EventHandler::EventHandler() : _fd(-1){};

EventHandler::EventHandler(int fd) : _fd(fd){};

void EventHandler::set_fd(int fd){

	_fd = fd;
};

int EventHandler::get_fd(){

	return _fd;
};

EventHandler::~EventHandler(){}

/* int EventHandler::fd(int mode, int fd){

	if (mode == SET)
		_fd = fd;

	return _fd;
} */

