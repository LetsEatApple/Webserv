/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AEventHandler.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:16:57 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/31 14:44:29 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "lib.h"

class Reactor;

class EventHandler{

protected:
	int				_fd;
	Reactor			*_reactor; // make protected?

public:
	EventHandler();
	EventHandler(int fd);
	void			reactor(Reactor *reactor);
	void			set_fd(int fd);
	int				get_fd();
	//int			fd(int mode, int fd);
	virtual int		handleRead() = 0;
	virtual int		handleWrite() = 0;
	virtual			~EventHandler();
};