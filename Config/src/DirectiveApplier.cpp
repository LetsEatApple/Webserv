#include "DirectiveApplier.hpp"

#include <cerrno>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <Utils.hpp>

namespace config
{
	void applyLocationPath(Location& loc, Block const& b)
	{
		std::string const& path = b.args[0];
		loc.path = utils::validatePath(path, b.name, b.line);
	}

	void applyAlias(Location& loc, Directive const& d)
	{
		std::string const& alias = d.args[0];
		loc.alias = utils::validatePath(alias, d.name, d.line);
	}

	void applyAutoindex(Location& loc, Directive const& d)
	{
		std::string const& autoindex = d.args[0];

		if (autoindex == "on")
			loc.autoindex = true;
		else if (autoindex == "off")
			loc.autoindex = false;
		else
			throw std::runtime_error(utils::errorMsg(
				"Unexpected value '" + autoindex +
				"' in 'autoindex' directive", d.line));
	}

	void applyIndex(Location& loc, Directive const& d)
	{
		std::string const& index = d.args[0];

		if (index.empty())
			throw std::runtime_error(utils::errorMsg(
				"Unexpected value '" + index +
				"' in 'index' directive", d.line));
		loc.index = index;
	}

	void applyUpload(Location& loc, Directive const& d)
	{
		std::string const& upload = d.args[0];

		if (upload.empty())
		{
			loc.upload = "";
			return;
		}
		loc.upload = utils::validatePath(upload, d.name, d.line);
	}

	void applyLimitExcept(Location& loc, Directive const& d)
	{
		std::set<std::string> methods;
		for (size_t i = 0; i < d.args.size(); ++i)
		{
			std::string const& method = d.args[i];

			if (method == "GET" || method == "POST" || method == "DELETE")
				methods.insert(method);
			else
				throw std::runtime_error(
					utils::errorMsg("Unexpected value '" + method +
					"' in 'limit_except' directive", d.line));
		}
		loc.limit_except = methods;
	}

	void applyClientMaxBodySize(Location& loc, Directive const& d)
	{
		std::string const& clientMaxBodySize = d.args[0];

		if (clientMaxBodySize.empty())
			throw std::runtime_error(utils::errorMsg(
				"Unexpected value '" + clientMaxBodySize +
				"' in 'client_max_body_size' directive", d.line));

		const char unit = clientMaxBodySize[clientMaxBodySize.size() - 1];
		size_t max = std::numeric_limits<size_t>::max();
		size_t scale = 1;
		std::string value = clientMaxBodySize;
		if (unit == 'k' || unit == 'K')
		{
			scale = 1024;
			max = std::numeric_limits<size_t>::max() / 1024;
			value = clientMaxBodySize.substr(0, clientMaxBodySize.size() - 1);
		}
		else if (unit == 'm' || unit == 'M')
		{
			max = std::numeric_limits<size_t>::max() / (1024 * 1024);
			scale = 1024 * 1024;
			value = clientMaxBodySize.substr(0, clientMaxBodySize.size() - 1);
		}

		if (!utils::isNumber(value) || value.empty())
			throw std::runtime_error(utils::errorMsg(
				"Unexpected value '" + clientMaxBodySize +
				"' in 'client_max_body_size' directive", d.line));

		errno = 0;
		char* end;
		const unsigned long s = std::strtoul(value.c_str(), &end, 10);
		if (errno == ERANGE || s > max || *end != '\0')
			throw std::runtime_error(utils::errorMsg(
				"Unexpected value '" + clientMaxBodySize +
				"' in 'client_max_body_size' directive", d.line));

		loc.client_max_body_size = s * scale;
	}

	void applyErrorPages(Location& loc, Directive const& d)
	{
		std::map<int, std::string> error_page;

		const std::string normalized = utils::validatePath(
			d.args[d.args.size() - 1], d.name, d.line);
		for (size_t i = 0; i < d.args.size() - 1; ++i)
		{
			char* end;
			errno = 0;
			const long s = std::strtol(d.args[0].c_str(), &end, 10);
			if (errno || *end != '\0' || s > std::numeric_limits<int>::max())
				throw std::runtime_error(
					utils::errorMsg("Unexpected value '" +
					d.args[0] + "' in 'error_page' directive",
					d.line));
			const int status = static_cast<int>(s);
			if (d.args[i].length() != 3 || status < 300 || status > 599)
				throw std::runtime_error(
					utils::errorMsg("Unexpected value '" +
					d.args[i] + "' in 'error_page' directive",
					d.line));
			error_page.insert(std::pair<int, std::string>(status, normalized));
		}
		loc.error_page = error_page;
	}

	void applyReturn(Location& loc, Directive const& d)
	{
		char* end;
		errno = 0;
		const long s = std::strtol(d.args[0].c_str(), &end, 10);
		if (errno || *end != '\0' || s > std::numeric_limits<int>::max())
			throw std::runtime_error(utils::errorMsg(
				"Unexpected value '" + d.args[0] +
				"' in 'error_page' directive", d.line));
		const int status = static_cast<int>(s);
		std::string normalized = d.args[d.args.size() - 1];
		if (normalized[0] == '/')
			normalized = utils::validatePath(d.args[d.args.size() - 1], d.name, d.line);
		if (d.args[0].length() != 3 || status < 100 || status > 599)
			throw std::runtime_error(
				utils::errorMsg("Unexpected value '" + d.args[0] +
				"' in '" + d.name + "' directive", d.line));
		loc.ret.insert(std::pair<int, std::string>(status, normalized));
	}

	void applyCGI(Location& loc, Directive const& d)
	{
		const std::string extension = d.args[0];
		if (extension[0] != '.')
			throw std::runtime_error(
				utils::errorMsg("Unexpected value '" + d.args[0] +
				"' in '" + d.name + "' directive", d.line));
		std::string normalized = d.args[1];
		if (normalized[0] == '/')
			normalized = utils::validatePath(d.args[1], d.name, d.line);
		loc.cgi.insert(std::pair<std::string, std::string>(extension, normalized));
	}
} // namespace config
