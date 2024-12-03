#include <libgs/http/client/session_pool.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::http::session_pool spool(libgs::execution::get_executor());
	asio::thread_pool pool(16);
#if 1
	libgs::dispatch([&]() -> asio::awaitable<void>
	{
		auto session = co_await spool.get(/*pool,*/
			{asio::ip::address::from_string("127.0.0.1"),8080}, libgs::use_awaitable
		);
		auto wbuf ="GET / HTTP/1.1\r\n"
		           "Host: 127.0.0.1:8080\r\n"
		           "\r\n";
		auto res = co_await asio::async_write(session.socket(), asio::buffer(wbuf, strlen(wbuf)), libgs::use_awaitable);
		spdlog::info("Sent {} bytes", res);

		char rbuf[8192] {0};
		res = co_await session.socket().async_read_some(asio::buffer(rbuf, 8192), libgs::use_awaitable);

		spdlog::info("Received {} bytes\n", res);
		spdlog::info("Response: {}", rbuf);

		pool.stop();
		libgs::execution::exit(0);
		co_return ;
	});
#else
	spool.get(/*pool,*/{asio::ip::address::from_string("127.0.0.1"),8080},
	[&pool](libgs::http::session_pool::session_t session, const std::error_code &error)
	{
	    static auto wbuf ="GET / HTTP/1.1\r\n"
	                      "Host: 127.0.0.1:8080\r\n"
	                      "\r\n";
	    asio::async_write(session.socket(), asio::buffer(wbuf, strlen(wbuf)),
	    [&pool, session = std::move(session)](const std::error_code &error, size_t wres) mutable
	    {
	        spdlog::info("Sent {} bytes", wres);
	        static char rbuf[8192] {0};

	        session.socket().async_read_some(asio::buffer(rbuf, 8192), [&pool](const std::error_code &error, size_t rres)
	        {
	            spdlog::info("Received {} bytes\n", rres);
	            spdlog::info("Response: {}", rbuf);

	            pool.stop();
	            libgs::execution::exit(0);
	        });
	    });
	});
#endif
	return libgs::execution::exec();
}
