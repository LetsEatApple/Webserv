/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:05:42 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/10 19:42:31 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "reactor.hpp"
#include "AEventHandler.hpp"
#include "handler.hpp"
#include "Router.hpp"
#include "routes.hpp"

int main(int ac, char **av) {
    // PARSING
    if (ac != 2)
        return error(0);

	#ifdef DEBUG
		std::cout << "DEBUG MODE ON" << std::endl;
	#endif

    config::Listeners listeners;
    try {
        listeners = config::buildConfig(av[1]);
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // LOW-LEVEL NETWORKING
    try {
        Server server(listeners);
        if (!server.createServer())
            return 1;
        // // change later
        // Router router;
        //


        // Reactor reactor(server, &router);
        Reactor reactor;
        if (!reactor.init(server.get_listeners()))
            return 1;

        // Event loop
        for(;;) {
            if (!reactor.handleEvents()){
                if (errno == EINTR)
                    continue ;
                return error(7);
            }
            if (reactor.stop(GET))
                break ;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}
