#include <stdexcept>
#include "AST.hpp"
#include "BlockValidators.hpp"
#include "DirectiveValidators.hpp"
#include "Utils.hpp"

namespace validator
{
	static DirectiveType directiveType(const std::string& name)
	{
		if (name == "client_max_body_size")
			return DIR_CLIENT_MAX_BODY_SIZE;
		else if (name == "error_page")
			return DIR_ERROR_PAGE;
		else if (name == "listen")
			return DIR_LISTEN;
		else if (name == "server_name")
			return DIR_SERVER_NAME;
		else if (name == "limit_except")
			return DIR_LIMIT_EXCEPT;
		else if (name == "alias")
			return DIR_ALIAS;
		else if (name == "autoindex")
			return DIR_AUTOINDEX;
		else if (name == "index")
			return DIR_INDEX;
		else if (name == "upload")
			return DIR_UPLOAD;
		else if (name == "return")
			return DIR_RETURN;
		else if (name == "cgi")
			return DIR_CGI;
		else
			return DIR_UNKNOWN;
	}

	static void validateDirective(const Directive& d, const ContextType ctx,
								  ValidationMask& seen)
	{
		const DirectiveType dir = directiveType(d.name);
		switch (dir)
		{
		case DIR_ERROR_PAGE:
			validateErrorPage(d, seen);
			break;
		case DIR_CLIENT_MAX_BODY_SIZE:
			validateClientMaxBodySize(d, seen);
			break;
		case DIR_LISTEN:
			validateListen(d, ctx, seen);
			break;
		case DIR_SERVER_NAME:
			validateServerName(d, ctx, seen);
			break;
		case DIR_RETURN:
			validateReturn(d, ctx, seen);
			break;
		case DIR_ALIAS:
			validateAlias(d, ctx, seen);
			break;
		case DIR_LIMIT_EXCEPT:
			validateLimitExcept(d, ctx, seen);
			break;
		case DIR_AUTOINDEX:
			validateAutoindex(d, seen);
			break;
		case DIR_INDEX:
			validateIndex(d, seen);
			break;
		case DIR_UPLOAD:
			validateUpload(d, seen);
			break;
		case DIR_CGI:
			validateCGI(d, ctx, seen);
			break;
		case DIR_UNKNOWN:
			throw std::runtime_error(
				utils::errorMsg("Unknown directive '" + d.name + "'", d.line));
		}
	}

	static void validateLocation(const Block& b)
	{
		ValidationMask seen = 0;
		for (size_t i = 0; i < b.directives.size(); ++i)
			validateDirective(b.directives[i], CTX_LOCATION, seen);

		if (b.args.size() != 1)
			throw std::runtime_error(utils::errorMsg(
				"Expected exactly 1 argument for '" +
				b.name + "'", b.line));

		if (!b.blocks.empty())
			throw std::runtime_error(utils::errorMsg(
				"'" + b.blocks[0].name + "' is not allowed here", b.line));
	}

	static void validateServer(const Block& b)
	{
		ValidationMask seen = 0;

		for (size_t i = 0; i < b.directives.size(); ++i)
			validateDirective(b.directives[i], CTX_SERVER, seen);

		for (size_t i = 0; i < b.blocks.size(); ++i)
		{
			if (b.blocks[i].name != "location")
				throw std::runtime_error(utils::errorMsg(
					"'" + b.blocks[i].name + "' directive is not allowed here",
					b.blocks[i].line));
			validateLocation(b.blocks[i]);
		}
	}

	static void validateHTTP(const Block& b)
	{
		ValidationMask seen = 0;

		for (size_t i = 0; i < b.directives.size(); ++i)
			validateDirective(b.directives[i], CTX_HTTP, seen);

		for (size_t i = 0; i < b.blocks.size(); ++i)
		{
			if (b.blocks[i].name != "server")
				throw std::runtime_error(utils::errorMsg(
					"'" + b.blocks[i].name + "' directive is not allowed here",
					b.blocks[i].line));
			seen |= HAS_SERVER;
			validateServer(b.blocks[i]);
		}
		if (!(seen & HAS_SERVER))
			throw std::runtime_error(
				utils::errorMsg("Expected 'server' directive at top level", b.line));
	}

	void validate(const Block& http)
	{
		if (http.name != "http")
			throw std::runtime_error(utils::errorMsg(
				"Unexpected root context name '" +
				http.name + "'", http.line));
		validateHTTP(http);
	}
} // namespace validator
