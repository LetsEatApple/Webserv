//
// Created by mkugan on 3/5/26.
//

#include "ConfigPrinter.hpp"

#include <iomanip>

namespace config
{
    static const char* RESET = "\033[0m";
    static const char* BOLD = "\033[1m";

    static const char* RED = "\033[31m";
    static const char* GREEN = "\033[32m";
    static const char* YELLOW = "\033[33m";
    static const char* BLUE = "\033[34m";
    static const char* MAGENTA = "\033[35m";
    static const char* CYAN = "\033[36m";
    static const char* GRAY = "\033[90m";

    static const int KEY_WIDTH = 20;

    std::ostream& operator<<(std::ostream& out, Location const& l)
    {
        out << BOLD << BLUE << "      location {" << RESET << std::endl;

        out << "        " << CYAN << std::left << std::setw(KEY_WIDTH)
            << "path" << RESET << ": " << l.path << std::endl;

        out << "        " << CYAN << std::left << std::setw(KEY_WIDTH)
            << "alias" << RESET << ": " << l.alias << std::endl;

        out << "        " << CYAN << std::left << std::setw(KEY_WIDTH)
            << "autoindex" << RESET << ": "
            << (l.autoindex ? GREEN : RED) << (l.autoindex ? "on" : "off")
            << RESET << std::endl;

        out << "        " << CYAN << std::left << std::setw(KEY_WIDTH)
            << "client_max_body_size" << RESET << ": " << YELLOW
            << l.client_max_body_size << RESET << std::endl;

        out << "        " << CYAN << std::left << std::setw(KEY_WIDTH)
            << "index" << RESET << ": " << l.index << std::endl;

        if (l.upload.empty())
        {
            out << "        " << CYAN << std::left << std::setw(KEY_WIDTH)
                << "upload" << RESET << ": " << RED << "off" << RESET << std::endl;
        }
        else
        {
            out << "        " << CYAN << std::left << std::setw(KEY_WIDTH)
                << "upload" << RESET << ": " << l.upload << std::endl;
        }

        out << "        " << CYAN << std::left << std::setw(KEY_WIDTH)
            << "limit_except" << RESET << ":";
        for (std::set<std::string>::iterator it = l.limit_except.begin();
             it != l.limit_except.end(); ++it)
        {
            out << " " << *it;
        }
        out << std::endl;

        out << "        " << CYAN << "error_page" << RESET << " {";
        if (!l.error_page.empty())
            out << std::endl;
        for (std::map<int, std::string>::const_iterator it = l.error_page.begin();
             it != l.error_page.end(); ++it)
        {
            out << "            " << YELLOW << it->first << RESET << " "
                << it->second << std::endl;
        }
        if (!l.error_page.empty())
            out << "      ";
        out << "    }" << std::endl;

        out << "        " << CYAN << "return" << RESET << " {";
        if (!l.ret.empty())
            out << std::endl;
        for (std::map<int, std::string>::const_iterator it = l.ret.begin();
             it != l.ret.end(); ++it)
        {
            out << "            " << YELLOW << it->first << RESET << " "
                << it->second << std::endl;
        }
        if (!l.ret.empty())
            out << "      ";
        out << "    }" << std::endl;

        out << "        " << GRAY << "cgi" << " {";
        if (!l.cgi.empty())
            out << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = l.cgi.begin();
             it != l.cgi.end(); ++it)
        {
            out << "            " << it->first << " "
                << it->second << std::endl;
        }
        if (!l.cgi.empty())
            out << "      ";
        out << "    }" << std::endl;

        out << "      }" << RESET << std::endl;

        return out;
    }

    std::ostream& operator<<(std::ostream& out, Server const& s)
    {
        out << BOLD << MAGENTA << "    server {" << RESET << std::endl;
        out << BOLD << BLUE << "      server_name:" << RESET;
        for (size_t i = 0; i < s.server_names.size(); ++i)
        {
            out << " " << s.server_names[i];
        }
        out << std::endl;
        for (size_t i = 0; i < s.locations.size(); ++i)
        {
            out << s.locations[i];
        }
        out << BOLD << MAGENTA << "    }" << RESET << std::endl;
        return out;
    }

    std::ostream& operator<<(std::ostream& out, Listeners const& l)
    {
        for (Listeners::const_iterator it = l.begin(); it != l.end(); ++it)
        {
            out << BOLD << BLUE << "[" << RESET << std::endl;
            out << "  " << BOLD << YELLOW << it->first.first << ":"
                << it->first.second << " {" << RESET << std::endl;
            for (size_t i = 0; i < it->second.size(); ++i)
                out << it->second[i];
            out << BOLD << YELLOW << "  }" << RESET << std::endl;
            out << BOLD << BLUE << "]" << RESET << std::endl;
        }
        return out;
    }
} // namespace config
