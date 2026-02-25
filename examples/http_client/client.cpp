#include <libgs/http_nt/client.h>
#include <spdlog/spdlog.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::http_nt::client client;
	libgs::http_nt::request_arg arg;

	auto context = client.make_get({"http://baidu.com", arg});

	return 0;
}
