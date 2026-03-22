#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "AST.hpp"
#include "BlockValidators.hpp"
#include "ConfigPrinter.hpp"
#include "DirectiveApplier.hpp"
#include "Lexer.hpp"
#include "ListenParser.hpp"
#include "Parser.hpp"
#include "Utils.hpp"

namespace config
{
	void applyDirective(Location& loc, Directive const& d)
	{
		if (d.name == "alias")
			applyAlias(loc, d);
		else if (d.name == "autoindex")
			applyAutoindex(loc, d);
		else if (d.name == "index")
			applyIndex(loc, d);
		else if (d.name == "client_max_body_size")
			applyClientMaxBodySize(loc, d);
		else if (d.name == "upload")
			applyUpload(loc, d);
		else if (d.name == "limit_except")
			applyLimitExcept(loc, d);
		else if (d.name == "error_page")
			applyErrorPages(loc, d);
		else if (d.name == "return")
			applyReturn(loc, d);
		else if (d.name == "cgi")
			applyCGI(loc, d);
	}

	Location buildLocation(const Block& location, const Location& server_default_location)
	{
		Location default_location = server_default_location;
		applyLocationPath(default_location, location);
		for (size_t i = 0; i < location.directives.size(); ++i)
		{
			const Directive& d = location.directives[i];
			applyDirective(default_location, d);
		}
		return default_location;
	}

	bool hasRootLocation(const std::vector<Location>& locations)
	{
		for (size_t i = 0; i < locations.size(); ++i)
			if (locations[i].path == "/")
				return true;
		return false;
	}

	static bool sortLocation(const Location& a, const Location& b)
	{
		return a.path.size() > b.path.size();
	}

	void buildServers(Listeners& listeners, const Block& server,
					  const Location& http_default_location)
	{
		Location server_default_location = http_default_location;
		std::vector<ListenerKey> listens;
		std::vector<std::string> server_names;
		ListenerKey defaultKey;
		defaultKey.first = "0.0.0.0";
		defaultKey.second = 8000;
		for (size_t i = 0; i < server.directives.size(); ++i)
		{
			const Directive& d = server.directives[i];

			if (d.name == "listen")
				listens.push_back(makeListen(d));
			else if (d.name == "server_name")
				server_names = d.args;
			else
				applyDirective(server_default_location, d);
		}

		if (server_names.empty())
			server_names.push_back("");

		if (listens.empty())
		{
			Server s;
			s.server_names = server_names;

			for (size_t j = 0; j < server.blocks.size(); ++j)
			{
				Location loc = buildLocation(server.blocks[j], server_default_location);
				s.locations.push_back(loc);
			}
			if (!hasRootLocation(s.locations))
				s.locations.push_back(server_default_location);

			std::set<std::string> existing_names;
			for (size_t j = 0; j < listeners[defaultKey].size(); ++j)
			{
				const Server& existing_server = listeners[defaultKey][j];
				for (size_t l = 0; l < existing_server.server_names.size(); ++l)
				{
					existing_names.insert(existing_server.server_names[l]);
				}
			}
			for (size_t k = 0; k < s.server_names.size(); ++k)
			{
				if (existing_names.find(s.server_names[k]) != existing_names.end())
				{
					throw std::runtime_error(
						"conflicting server name '" + s.server_names[k] + "' on 127.0.0.1:8000");
				}
			}
			listeners[defaultKey].push_back(s);
		}

		for (size_t i = 0; i < listens.size(); ++i)
		{
			Server s;
			s.server_names = server_names;

			for (size_t j = 0; j < server.blocks.size(); ++j)
			{
				Location loc = buildLocation(server.blocks[j], server_default_location);
				s.locations.push_back(loc);
			}
			if (!hasRootLocation(s.locations))
				s.locations.push_back(server_default_location);
			std::sort(s.locations.begin(), s.locations.end(), sortLocation);

			std::set<std::string> existing_names;
			for (size_t j = 0; j < listeners[listens[i]].size(); ++j)
			{
				const Server& existing_server = listeners[listens[i]][j];
				for (size_t l = 0; l < existing_server.server_names.size(); ++l)
				{
					existing_names.insert(existing_server.server_names[l]);
				}
			}
			for (size_t k = 0; k < s.server_names.size(); ++k)
			{
				if (existing_names.find(s.server_names[k]) != existing_names.end())
				{
					throw std::runtime_error(
						"conflicting server name '" +
						s.server_names[k] + "' on " +
						listens[i].first + ":" +
						utils::toString(listens[i].second));
				}
			}
			listeners[listens[i]].push_back(s);
		}
	}

	Listeners buildConfig(const std::string& path)
	{
		Listeners listeners;
		Lexer lexer(path);
		Parser parser(lexer);
		Block http = parser.parse();
		validator::validate(http);

		Location http_default_location;
		std::map<int, std::string> ret;
		std::map<std::string, std::string> cgi;
		std::set<std::string> limit_except;
		limit_except.insert("GET");
		limit_except.insert("POST");
		limit_except.insert("DELETE");
		std::map<int, std::string> error_page;
		http_default_location.path = "/";
		http_default_location.alias = "/var/www";
		http_default_location.index = "index.html";
		http_default_location.ret = ret;
		http_default_location.client_max_body_size = 1024 * 1024;
		http_default_location.error_page = error_page;
		http_default_location.autoindex = false;
		http_default_location.upload = "";
		http_default_location.limit_except = limit_except;
		http_default_location.cgi = cgi;

		for (size_t i = 0; i < http.directives.size(); ++i)
			applyDirective(http_default_location, http.directives[i]);
		for (size_t i = 0; i < http.blocks.size(); ++i)
			buildServers(listeners, http.blocks[i], http_default_location);

		std::cout << listeners;
		return listeners;
	}
} // namespace config
