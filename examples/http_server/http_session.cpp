#include <libgs/http/server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	asio::ip::tcp::acceptor acceptor(libgs::get_executor());
	constexpr unsigned short port = 12345;

	libgs::http::server server(std::move(acceptor));
	server.bind({libgs::ip_type::v4, port})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::http::server::context_t &context) -> libgs::awaitable<void>
	{
		auto session = context.session();
		spdlog::debug("session: '{}': <{}>", session->id(), session);
		co_return ;
	})
	.on_service_error([](libgs::http::server::context_t&, const std::exception &ex)
	{
		spdlog::error(ex);
		return true;
	})
	.on_server_error([](std::error_code error)
	{
		spdlog::error(error);
		libgs::exit(-1);
		return true;
	})
	.start();

	spdlog::info("HTTP Server started ({}) ...", port);
	return libgs::exec();
}