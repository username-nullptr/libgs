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
		auto connection = co_await cpool.get(/*pool,*/
			{asio::ip::make_address("127.0.0.1"), 8080}, libgs::use_awaitable
		);
		if( not connection )
		{
			spdlog::error("Failed get connection", connection.error());
			co_return ;
		}
		auto wbuf ="GET / HTTP/1.1\r\n"
		           "Host: 127.0.0.1:8080\r\n"
		           "\r\n";
		auto res = co_await asio::async_write(connection->socket(),
			asio::buffer(wbuf, strlen(wbuf)), libgs::use_awaitable
		);
		spdlog::info("Sent {} bytes", res);

		char rbuf[8192] {0};
		res = co_await connection->socket()
			.async_read_some(asio::buffer(rbuf, 8192), libgs::use_awaitable);

		spdlog::info("Received {} bytes\n", res);
		spdlog::info("Response: {}", rbuf);

		pool.stop();
		libgs::exit(0);
		co_return ;
	});
#else
	cpool.get(/*pool,*/{asio::ip::make_address("127.0.0.1"),8080},
	[&pool](libgs::http::connection_pool::con_expected_t connection)
	{
		if( not connection )
		{
			spdlog::error("Failed get connection", connection.error());
			return ;
		}
		static auto wbuf ="GET / HTTP/1.1\r\n"
	                      "Host: 127.0.0.1:8080\r\n"
	                      "\r\n";
	    asio::async_write(connection->socket(), asio::buffer(wbuf, strlen(wbuf)),
	    [&pool, connection = std::move(connection)](const std::error_code &error, size_t wres) mutable
	    {
	    	if( error )
	    	{
				spdlog::error("Failed write to server", error);
				return ;
			}
	        spdlog::info("Sent {} bytes", wres);
	        static char rbuf[8192] {0};

	        connection->socket().async_read_some(asio::buffer(rbuf, 8192),
	        [&pool](const std::error_code &error, size_t rres)
	        {
	        	LIBGS_UNUSED(error);
	            spdlog::info("Received {} bytes\n", rres);
	            spdlog::info("Response: {}", rbuf);

	            pool.stop();
	            libgs::exit(0);
	        });
	    });
	});
#endif
	return libgs::exec();
}