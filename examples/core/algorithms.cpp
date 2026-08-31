#include <libgs/core/algorithm.h>
#include <libgs/core/mime_type.h>
#include <iostream>

int main()
{
	std::cout << "MIME type: " << libgs::mime_type::get("index.html") << '\n';

	libgs::sha1 digest("Hello from LibGS");
	std::cout << "SHA-1: " << digest.finalize().hex() << '\n';
	std::cout << "UUID: " << libgs::uuid::generate().to_string() << '\n';

	auto weight = libgs::wildcard_match("lib*.so*", "libexample.so.1");
	std::cout << "Wildcard match weight: " << weight << '\n';
	return 0;
}
