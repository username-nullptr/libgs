#include <libgs/utils/modules.h>
#include <iostream>

LIBGS_UTILS_MODULE_INIT(
	"api",
	{.parents = {"cache"}}, []{
		std::cout << "api initialized after cache\n";
	}
);
