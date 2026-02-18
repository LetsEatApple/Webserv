#include "MimeTypes.hpp"

static std::string getExtension(const std::string& path) {
	std::string::size_type dot = path.rfind('.');
	if (dot == std::string::npos)
		return "";
	std::string::size_type slash = path.rfind('/');
	if (slash != std::string::npos && dot < slash)
		return "";
	return path.substr(dot);
}

std::string getMimeType(const std::string& path) {
	std::string ext = getExtension(path);

	if (ext == ".html" || ext == ".htm") return "text/html";
	if (ext == ".css")  return "text/css";
	if (ext == ".js")   return "application/javascript";
	if (ext == ".json") return "application/json";
	if (ext == ".xml")  return "application/xml";
	if (ext == ".txt")  return "text/plain";
	if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
	if (ext == ".png")  return "image/png";
	if (ext == ".gif")  return "image/gif";
	if (ext == ".svg")  return "image/svg+xml";
	if (ext == ".ico")  return "image/x-icon";
	if (ext == ".pdf")  return "application/pdf";
	return "application/octet-stream";
}
