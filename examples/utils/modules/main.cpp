#include <libgs/utils/modules.h>
#include <iostream>

int main()
{
	std::cout << "Registered module graph:\n"
		<< libgs::utils::modules::sprint() << '\n';

	libgs::utils::modules::do_init();

	std::cout << "All modules initialized\n";
	return 0;
}
