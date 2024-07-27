#include <libgs/http/server/request.h>
#include <libgs/http/server/response.h>
#include <libgs/http/server/session_set.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::http::session_set ss;

	return 0;
}
