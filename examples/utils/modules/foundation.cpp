#include <libgs/utils/modules.h>
#include <iostream>

LIBGS_UTILS_MODULE_INIT("foundation", []
{
	std::cout << "foundation initialized\n";
});
