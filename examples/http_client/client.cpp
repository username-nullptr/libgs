#include <libgs/http_nt/client.h>
#include <spdlog/spdlog.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::http_nt::client client;

	return 0;
}
