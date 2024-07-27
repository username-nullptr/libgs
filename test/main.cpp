#include <libgs/http/server/request.h>
#include <libgs/http/server/response.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);

	asio::ip::tcp::socket socket(libgs::execution::get_executor());

	libgs::http::request_parser parser;

	libgs::http::tcp_server_request request(std::move(socket), parser);

	libgs::http::tcp_server_response response(std::move(request));

	std::error_code error;

	response.write(asio::buffer("123",3));
	response.write(asio::buffer("123",3), error);
	response.write(error);
	response.write();

	response.async_write(asio::buffer("123",3), [](size_t,std::error_code){});
	response.async_write(asio::buffer("123",3), asio::use_awaitable|error);
	response.async_write([](size_t,std::error_code){});
	response.async_write(asio::use_awaitable|error);


	response.redirect("aaa", libgs::http::redirect::see_other);
	response.redirect("aaa", libgs::http::redirect::see_other, error);
	response.redirect("aaa", error);
	response.redirect("aaa");

	response.async_redirect("aaa", libgs::http::redirect::see_other, [](size_t,std::error_code){});
	response.async_redirect("aaa", libgs::http::redirect::see_other, asio::use_awaitable|error);
	response.async_redirect("aaa", [](size_t,std::error_code){});
	response.async_redirect("aaa", asio::use_awaitable|error);


	response.send_file("file", {2,4});
	response.send_file("file", {5}, error);
	response.send_file("file", error);
	response.send_file("file");

	response.async_send_file("file", {3,6}, [](size_t,std::error_code){});
	response.async_send_file("file", {4}, asio::use_awaitable|error);
	response.async_send_file("file", [](size_t,std::error_code){});
	response.async_send_file("file", asio::use_awaitable|error);


	response.chunk_end({{"key","value"}});
	response.chunk_end({{"key","value"}}, error);
	response.chunk_end(error);
	response.chunk_end();

	response.async_chunk_end({{"key","value"}}, [](size_t,std::error_code){});
	response.async_chunk_end({{"key","value"}}, asio::use_awaitable|error);
	response.async_chunk_end([](size_t,std::error_code){});
	response.async_chunk_end(asio::use_awaitable|error);

	return 0;
}
