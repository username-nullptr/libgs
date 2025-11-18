#include "process.h"
#include <libgs/core/app_utls.h>

namespace libgs::utils::detail
{

sys_expected<uint64_t> process::set_single(std::string_view key)
{
	return app::home_directory().and_then([&](const auto &path) mutable {
		return set_single(path, key);
	});
}

} //namespace libgs::utils::detail