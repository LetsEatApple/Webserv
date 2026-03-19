#ifndef MULTIPART_HPP
#define MULTIPART_HPP

#include <string>
#include <vector>

class Multipart {
	public:
		Multipart(const std::string& body, const std::string& boundary, const std::string& uploadDir);
		bool parse();
		void addUploadedFile(const std::string& filename);
		const std::vector<std::string>& getUploadedFiles() const;
		unsigned int getFields() const;
	private:
		std::string _body;
		std::string _boundary;
		std::string _uploadDir;
		std::vector<std::string> _uploadedFiles;
		unsigned int _fields;

		bool handlePart(const std::string& part);
};

#endif // !MULTIPART_HPP

