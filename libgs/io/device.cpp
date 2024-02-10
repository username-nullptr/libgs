#include "device.h"

namespace libgs::io
{

device::~device() = default;

void device::async_read_some(std::string &buf, size_t size, rw_callback_t callback) noexcept
{
	auto ptr = std::make_shared<char[]>(size);
	async_read_some(ptr.get(), size, [ptr, &buf, callback = std::move(callback)](size_t size, const error_code &error)
	{
		buf = std::string(ptr.get(), size);
		callback(size, error);
	});
}

size_t device::read_some(std::string &buf, size_t size, error_code *error) noexcept
{
	auto ptr = std::make_shared<char[]>(size);
	auto res = read_some(ptr.get(), size, error);
	buf = std::string(ptr.get(), size);
	return res;
}

awaitable<size_t> device::co_read_some(std::string &buf, size_t size, error_code *error) noexcept
{
	auto ptr = std::make_shared<char[]>(size);
	auto res = co_await co_read_some(ptr.get(), size, error);
	buf = std::string(ptr.get(), size);
	co_return res;
}

void device::async_read_some(std::wstring &buf, size_t size, rw_callback_t callback) noexcept
{
	auto ptr = std::make_shared<char[]>(size);
	async_read_some(ptr.get(), size, [ptr, &buf, callback = std::move(callback)](size_t size, const error_code &error)
	{
		buf = mbstowcs(std::string(ptr.get(), size));
		callback(size, error);
	});
}

size_t device::read_some(std::wstring &buf, size_t size, error_code *error) noexcept
{
	auto ptr = std::make_shared<char[]>(size);
	auto res = read_some(ptr.get(), size, error);
	buf = mbstowcs(std::string(ptr.get(), size));
	return res;
}

awaitable<size_t> device::co_read_some(std::wstring &buf, size_t size, error_code *error) noexcept
{
	auto ptr = std::make_shared<char[]>(size);
	auto res = co_await co_read_some(ptr.get(), size, error);
	buf = mbstowcs(std::string(ptr.get(), size));
	co_return res;
}

void device::async_write_some(const std::string &buf, rw_callback_t callback) noexcept
{
	async_write_some(buf.c_str(), buf.size(), std::move(callback));
}

size_t device::write_some(const std::string &buf, error_code *error) noexcept
{
	return write_some(buf.c_str(), buf.size(), error);
}

awaitable<size_t> device::co_write_some(const std::string &buf, error_code *error) noexcept
{
	co_return co_await co_write_some(buf.c_str(), buf.size(), error);
}

void device::async_write_some(const std::wstring &buf, rw_callback_t callback) noexcept
{
	auto tmp = wcstombs(buf);
	async_write_some(tmp.c_str(), tmp.size(), std::move(callback));
}

size_t device::write_some(const std::wstring &buf, error_code *error) noexcept
{
	auto tmp = wcstombs(buf);
	return write_some(tmp.c_str(), tmp.size(), error);
}

awaitable<size_t> device::co_write_some(const std::wstring &buf, error_code *error) noexcept
{
	auto tmp = wcstombs(buf);
	co_return co_await co_write_some(tmp.c_str(), tmp.size(), error);
}

device::await_bool_t device::co_wait_writeable(const duration &ms, error_code *error) noexcept
{
	co_return co_await co_thread([&]()-> bool {
		return wait_writeable(ms, error);
	});
}

device::await_bool_t device::co_wait_readable(const duration &ms, error_code *error) noexcept
{
	co_return co_await co_thread([&]() -> bool {
		return wait_readable(ms, error);
	});
}

} //namespace libgs::io
