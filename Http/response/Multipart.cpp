#include "Multipart.hpp"
#include <fstream>
#include <sstream>
#include <string>

Multipart::Multipart(const std::string& body,
	const std::string& boundary,
	const std::string& uploadDir):
	_body(body), _boundary("--" + boundary), _uploadDir(uploadDir), _uploadedFiles(), _fields(0) {
}

void Multipart::addUploadedFile(const std::string& filename)
{
	_uploadedFiles.push_back(filename);
}

const std::vector<std::string>& Multipart::getUploadedFiles() const
{
	return _uploadedFiles;
}

unsigned int Multipart::getFields() const {
	return _fields;
}

bool Multipart::parse() {
	std::size_t pos = 0;
	while (true) {
		const std::size_t boundaryPos = _body.find(_boundary, pos);
		if (boundaryPos == std::string::npos)
			break;
		if (_body.compare(boundaryPos, _boundary.size() + 2, _boundary + "--") == 0)
			break;
		std::size_t partStart = boundaryPos + _boundary.size();
		if (_body.compare(partStart, 2, "\r\n") == 0)
			partStart += 2;
		std::size_t nextBoundary = _body.find(_boundary, partStart);
		if (nextBoundary == std::string::npos)
			nextBoundary = _body.size();
		std::string part = _body.substr(partStart, nextBoundary - partStart);
		if (!handlePart(part))
			return false;
		pos = nextBoundary;
	}
	return true;
}

bool Multipart::handlePart(const std::string& part)
{
	std::size_t headerEnd = part.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return false;
	std::string headerStr = part.substr(0, headerEnd);
	std::string body = part.substr(headerEnd + 4);
	if (body.size() >= 2 && body.substr(body.size() - 2) == "\r\n")
		body.erase(body.size() - 2);
	std::string filename;
	std::istringstream headers(headerStr);
	std::string line;
	while (std::getline(headers, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.find("Content-Disposition") != std::string::npos) {
			std::istringstream params(line);
			std::string token;
			while (std::getline(params, token, ';')) {
				while (!token.empty() && token[0] == ' ')
					token.erase(0, 1);
				if (token.find("filename=") == 0) {
					std::string val = token.substr(9);
					if (val.size() >= 2 && val[0] == '"' && val[val.size() - 1] == '"')
						val = val.substr(1, val.size() - 2);
					filename = val;
				}
			}
		}
	}
	if (filename.empty()) {
		_fields++;
		return true;
	}
	if (filename.find('/') != std::string::npos || filename.find("..") != std::string::npos)
		return false;
	std::ofstream out((_uploadDir + filename).c_str(), std::ios::binary);
	if (!out)
		return false;
	out.write(body.c_str(), static_cast<std::streamsize>(body.size()));
	out.close();
	_fields++;
	addUploadedFile(filename);
	return true;
}
