/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:05:42 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/29 13:02:49 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "reactor.hpp"
#include "AEventHandler.hpp"
#include "handler.hpp"

int	error(int err){

	std:: cout << "Error: ";
	switch (err)
	{
	case 0:
		std::cout << "Usage: ./webserv config.conf" << std::endl;
		return false;
	case 1:
		std::cout << "Creating socket failed: ";
		break;
	case 2:
		std::cout << "Binding socket failed: ";
		break;
	case 3:
		std::cout << "Listening to socket failed: ";
		break;
	case 4:
		std::cout << "Epoll creation failed: ";
		break;
	case 5:
		std::cout << "Creating signal fd failed: ";
		break;
	case 6:
		std::cout << "epoll_ctl on listening socket failed: ";
		break;
	case 7:
		std::cout << "epoll_wait failed: ";
		break;
	default:
		break;
	}
	std::cout << strerror(errno) << std::endl;
	return false;
}

int main(int ac, char **av){

	// PARSING
	if (ac != 2)
		return error(0);

	config::Listeners listeners;
	try {
		listeners = config::buildConfig(av[1]);
	}
	catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	// LOW-LEVEL NETWORKING
	try
	{
		Server server(listeners);
		if (!server.createServer())
				return 1;
		
		Reactor reactor(server);
		if (!reactor.init())
			return 1;
		
		for(;;){
			if (!reactor.handleEvents()){
				if (errno == EINTR)
					continue ;
				return error(7);
			}
			if (reactor.stop(GET))
				break ;
		}
		return 0;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
}
