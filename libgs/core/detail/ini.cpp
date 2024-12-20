#include <libgs/core/ini.h>

namespace libgs::detail
{

static ini_error_category

g_invalid_group {
	"invalid_group", "Invalid group"
},
g_no_group_specified {
	"no_group_specified", "No group specified"
},
g_invalid_key_value_line {
	"invalid_key_value_line", "Invalid key-value line"
},
g_key_is_empty {
	"invalid_key_value_line", "Invalid key-value line: Key is empty"
},
g_invalid_value {
	"invalid_key_value_line", "Invalid key-value line: Invalid value"
};

ini_error_category &ini_invalid_group() noexcept
{
	return g_invalid_group;
}

ini_error_category &ini_no_group_specified() noexcept
{
	return g_no_group_specified;
}

ini_error_category &ini_invalid_key_value_line() noexcept
{
	return g_invalid_key_value_line;
}

ini_error_category &ini_key_is_empty() noexcept
{
	return g_key_is_empty;
}

ini_error_category &ini_invalid_value() noexcept
{
	return g_invalid_value;
}

static struct io_work
{
	// Don't destruct it.
	// An exception occurs when the main function exits in Win10.
	asio::io_context *ioc = new asio::io_context();

	std::thread thread {[this]
	{
		const asio::io_context::work worker(*ioc); LIBGS_UNUSED(worker);
		ioc->run();
	}};
	~io_work()
	{
		ioc->stop();
		if( not thread.joinable() )
			return ;
		try {
			thread.join();
		} catch(...) {}
	}
}
g_io_work;

void ini_commit_io_work(std::function<void()> work)
{
	asio::post(*g_io_work.ioc, std::move(work));
}

} //namespace libgs::detail