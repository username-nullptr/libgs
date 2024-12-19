#include "ini.h"

#include "libgs/core/execution.h"

namespace libgs::detail
{

class LIBGS_DECL_HIDDEN ini_error_category::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl(const char *name, const char *desc) :
		m_name(name), m_desc(desc) {}

	const char *m_name;
	const char *m_desc;
};

ini_error_category::ini_error_category(const char *name, const char *desc) :
	m_impl(new impl(name, desc))
{

}

const char *ini_error_category::name() const noexcept override
{
	return m_impl->m_name;
}

std::string ini_error_category::message(int line) const override
{
	return std::format("Ini file parsing: syntax error: [line:{}]: {}.", line, m_impl->m_desc);
}

ini_error_category

ini_error_category::invalid_group {
	"invalid_group", "Invalid group"
},
ini_error_category::no_group_specified {
	"no_group_specified", "No group specified"
},
ini_error_category::invalid_key_value_line {
	"invalid_key_value_line", "Invalid key-value line"
},
ini_error_category::key_is_empty {
	"invalid_key_value_line", "Invalid key-value line: Key is empty"
},
ini_error_category::invalid_value {
	"invalid_key_value_line", "Invalid key-value line: Invalid value"
};

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

void ini_commit_io_work(std::function<void()> task)
{
	asio::post(*g_io_work.ioc, std::move(task));
}

} //namespace libgs::detail