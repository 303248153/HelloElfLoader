#pragma once
#include <string>
#include <fstream>

namespace HelloElfLoader {
	class Loader {
		std::ifstream fileStream_;

	public:
		Loader(const std::string& path);
		Loader(std::ifstream&& fileStream);
		void execute();
	};
}
