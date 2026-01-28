/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:05:42 by lhagemos          #+#    #+#             */
/*   Updated: 2026/01/24 17:41:17 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "reactor.hpp"
#include "AEventHandler.hpp"
#include "handler.hpp"
#include "Router.hpp"
#include "Request.hpp"
#include "Response.hpp"

static void handleDefault(Response::Writer& w, const Request& req) {
	(void)req;
	const char* body = "<html><body><h1>200 OK</h1></body></html>";
	int bodyLen = static_cast<int>(std::strlen(body));
	Headers h = Response::getDefaultHeaders(bodyLen);
	h.replace("content-type", "text/html");
	w.writeStatusLine(Response::StatusOk);
	w.writeHeaders(h);
	w.writeBody(body, bodyLen);
}

int	error(int err){

	std:: cout << "Error: ";
	switch (err)
	{
	case 0:
		std::cout << "Wrong/missing input: Program demands one configuration file as input!" << std::endl;
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

	if (ac < 2)
		return error(0); // later config ac != 2

	Router router;
	router.setDefault(handleDefault);

	Server server(ac -1, av, &router);
	if (!server.createServer())
		return false;

	Reactor reactor(server);
	if (!reactor.init())
		return false;

	// Event loop
	for(;;){
		if (!reactor.handleEvents()){
			if (errno == EINTR)
				continue ;
			return error(7);
		}
		if (reactor.stop(GET))
			break ;
	}
	return true;
}
