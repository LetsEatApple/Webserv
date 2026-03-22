/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:05:42 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/14 18:01:11 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <Logger.hpp>

#include "server.hpp"
#include "reactor.hpp"
#include "handler.hpp"

int main(int ac, char **av) {

	std::string status = "OFF";
	#ifdef DEBUG
		status = "ON";
	#endif
	std::cout << "\n\033[1;35mWEBSERV\033[0;37m\t"
	<< "\033[90m[DEBUG MODE " << status << "]\033[0m\n"
	<< std::endl;
	logInfo("Webserv starting", " ...");

	// PARSING
	if (ac != 2)
		return error("Usage: ./webserv config.conf", 0);
	
	config::Listeners listeners;
	try {
		listeners = config::buildConfig(av[1]);
	}
	catch (const std::exception &e) {
		return error(e.what(), 0);
	}
	logInfo("Loaded configuration file: ", av[1]);
	// LOW-LEVEL NETWORKING
	try {
		Server server(listeners);
		if (!server.createServer())
			return 1;

	// Reactor reactor(server, &router);
		Reactor reactor;
		if (!reactor.init(server.get_sessions()))
			return 1;

		// Event loop
		for(;;) {
			reactor.handleEvents();
			if (reactor.stop(GET))
				break ;
		}
	}
	catch(const std::exception& e)
	{
		return error(e.what(), 0);
	}
}
