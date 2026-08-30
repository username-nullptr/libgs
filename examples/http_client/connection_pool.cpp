#include <libgs/http/client/connection_pool.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::http::connection_pool cpool(libgs::get_executor());
	asio::thread_pool pool(16);
#if 0
	libgs::dispatch([&]() -> asio::awaitable<void>
	{
		using namespace libgs::operators;
		std::error_code error;

		auto lease = co_await cpool.get(/*pool,*/
			{ "127.0.0.1", 8080}, libgs::use_awaitable | error
		);
		if( error )
		{
			spdlog::error("Failed get connection", error);
			co_return ;
		}
		auto wbuf ="GET / HTTP/1.1\r\n"
		           "Host: 127.0.0.1:8080\r\n"
		           "\r\n";
		auto res = co_await lease->get().write (
			asio::buffer(wbuf, strlen(wbuf)), libgs::use_awaitable
		);
		spdlog::info("Sent {} bytes", res);

		char rbuf[8192] {0};
		res = co_await lease->get().read (
			asio::buffer(rbuf, 8192), libgs::use_awaitable
		);
		spdlog::info("Received {} bytes\n", res);
		spdlog::info("Response: {}", rbuf);

		pool.stop();
		libgs::exit(0);
		co_return ;
	});
#else
	cpool.get(/*pool,*/{ "127.0.0.1", 8080 },
	[&pool](const std::error_code &acquire_error,
		libgs::http::connection_pool::lease_ptr lease_result)
	{
		if( acquire_error )
		{
			spdlog::error("Failed get connection", acquire_error);
			return ;
		}
		static auto wbuf ="GET / HTTP/1.1\r\n"
	                      "Host: 127.0.0.1:8080\r\n"
	                      "\r\n";
		lease_result->get().write(asio::buffer(wbuf, strlen(wbuf)),
		[&event_loop = pool, lease = std::move(lease_result)]
		(const std::error_code &write_error, size_t wres) mutable
		{
			if( write_error )
			{
				spdlog::error("Failed write to server", write_error);
				return ;
			}
			spdlog::info("Sent {} bytes", wres);
			static char rbuf[8192] {0};

			lease->get().read(asio::buffer(rbuf, 8192),
			[&event_loop](const std::error_code &read_error, size_t rres)
			{
				LIBGS_UNUSED(read_error);
				spdlog::info("Received {} bytes\n", rres);
				spdlog::info("Response: {}", rbuf);

				event_loop.stop();
				libgs::exit(0);
			});
		});
	});
#endif
	return libgs::exec();
}
