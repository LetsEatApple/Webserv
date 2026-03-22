/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AEventHandler.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:16:57 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/10 19:29:46 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

class Reactor;

class EventHandler{

protected:
	int				_fd;
	Reactor			*_reactor; // make protected?

public:
	EventHandler();
	explicit EventHandler(int fd);
	void			reactor(Reactor *reactor);
	void			set_fd(int fd);
	int				get_fd() const;
	virtual int		handleRead() = 0;
	virtual int		handleWrite() = 0;
  virtual bool  checkTimeout();
	virtual			~EventHandler();
};
