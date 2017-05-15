#pragma once
#include <string>
#include <memory>

namespace HelloElfLoader {
	std::shared_ptr<void> resolveLibraryFunc(const std::string& name);
}
