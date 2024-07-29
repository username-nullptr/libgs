#include <libgs/http/server/request.h>
#include <libgs/http/server/response.h>
#include <libgs/http/server/session_set.h>
#include <libgs/http/server/context.h>
#include <libgs/http/server/aop.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

class my_session : public libgs::http::session
{
public:
	using libgs::http::session::basic_session;
};

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::http::session_set ss;

	auto session0 = ss.make();
	auto session1 = ss.make<my_session>();

	session0 = ss.get_or_make("aaa");
	session1 = ss.get_or_make<my_session>("aaa");

	session0 = ss.get("aaa");
	session1 = ss.get<my_session>("aaa");

	session0 = ss.get_or("aaa");
	session1 = ss.get_or<my_session>("aaa");

	ss.set_cookie_key("123123");

	return libgs::execution::exec();
}
