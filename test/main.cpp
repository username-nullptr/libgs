#include <iostream>
#include <libgs/core/algorithm.h>

using namespace std::chrono;

int main()
{
	auto uuid = libgs::uuid::generate().to_string();
	auto wuuid = libgs::wuuid::generate().to_string();

	std::cout << uuid << std::endl;
	std::wcout << wuuid << std::endl;

	auto sha1 = libgs::sha1(uuid).finalize();
	auto wsha1 = libgs::sha1(wuuid).finalize();

	std::cout << sha1.base64() << std::endl;
	std::cout << wsha1.base64() << std::endl;

	std::wcout << sha1.wbase64() << std::endl;
	std::wcout << wsha1.wbase64() << std::endl;

	return 0;
}
