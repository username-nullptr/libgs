#include <libgs/utils/modules.h>
#include <iostream>

LIBGS_UTILS_MODULE_INIT (
	"cache",
	{.parents = {"foundation"}}, []{
		std::cout << "cache initialized after foundation\n";
	}
);
