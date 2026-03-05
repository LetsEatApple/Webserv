//
// Created by mkugan on 3/5/26.
//

#include "ListenParser.hpp"

#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <stdexcept>
#include <Utils.hpp>
#include <sys/socket.h>

namespace config
{
    std::string getAddress(std::string& address, const std::string& port,
                           const Directive& d, const bool isV6)
    {
        if (address == "*")
            address = "0.0.0.0";
        if (address == "localhost")
            address = "127.0.0.1";

        struct addrinfo hints = {};
        struct addrinfo* res = NULL;

        hints.ai_family = isV6 ? AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(address.c_str(), port.c_str(), &hints, &res) != 0)
            throw std::runtime_error(utils::errorMsg(
                "Invalid address '" + address + "' of the 'listen' directive", d.line));

        freeaddrinfo(res);
        return address;
    }

    uint16_t getPort(std::string const& portStr, Directive const& d)
    {
        if (!utils::isNumber(portStr))
            throw std::runtime_error(utils::errorMsg("Invalid port in '" + d.args[0] +
                                                     "' of the 'listen' directive",
                                                     d.line));
        const long p = std::strtol(portStr.c_str(), NULL, 10);
        if (p < 1 || p > 65535)
            throw std::runtime_error(utils::errorMsg("Invalid port in '" + d.args[0] +
                                                     "' of the 'listen' directive",
                                                     d.line));
        return static_cast<uint16_t>(p);
    }

    std::pair<std::string, uint16_t> makeListen(Directive const& d)
    {
        std::string arg = d.args[0];
        std::string address = "0.0.0.0";
        uint16_t port = 8000;

        if (!arg.empty() && arg[0] == '[')
        {
            const size_t close = arg.find(']');
            if (close == std::string::npos || close == 1)
                throw std::runtime_error(
                    utils::errorMsg("Invalid IPv6 address format in '" + arg +
                                    "' of the 'listen' directive",
                                    d.line));

            address = arg.substr(1, close - 1);
            if (address == "*")
                throw std::runtime_error(
                    utils::errorMsg("Invalid IPv6 address format in '" + arg +
                                    "' of the 'listen' directive",
                                    d.line));

            if (close + 1 == arg.size())
            {
                const std::string portStr = utils::toString(port);
                address = getAddress(address, portStr, d, true);
            }
            else
            {
                if (arg[close + 1] != ':')
                    throw std::runtime_error(
                        utils::errorMsg("Invalid IPv6 address format in '" + arg +
                                        "' of the 'listen' directive",
                                        d.line));

                const std::string portStr = arg.substr(close + 2);
                port = getPort(portStr, d);
                address = getAddress(address, portStr, d, true);
            }
        }
        else if (arg.find(':') != std::string::npos)
        {
            const size_t colon = arg.find(':');

            if (arg.find(':', colon + 1) != std::string::npos)
                throw std::runtime_error(
                    utils::errorMsg("IPv6 address must be enclosed in [] in '" + arg +
                                    "' of the 'listen' directive",
                                    d.line));

            address = arg.substr(0, colon);
            const std::string portStr = arg.substr(colon + 1);
            port = getPort(portStr, d);
            address = getAddress(address, portStr, d, false);
        }

        else if (utils::isNumber(arg))
            port = getPort(arg, d);
        else
        {
            const std::string partStr = utils::toString(port);
            address = getAddress(arg, partStr, d, false);
        }

        return std::make_pair(address, port);
    }
} // namespace  config
