#include "stdafx.h"
#include "HelloElfLoader.h"
#include <iostream>

int main() {
	using namespace HelloElfLoader;
	try {
		Loader loader("../sample/a.out");
		loader.execute();
	}
	catch (const std::exception& ex) {
		std::cout << "error: " << ex.what() << std::endl;
	}
	return 0;
}
