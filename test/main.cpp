#include <libgs/http_nt/client.h>

#include <libgs/core/system/app_utls.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/execution.h>

#include <libgs/coro/utils.h>
#include <libgs/utils/modules.h>

#include <libgs/utils/signal_slot.h>
#include <libgs/utils/process.h>
#include <libgs/utils/logger.h>

#include <spdlog/spdlog.h>
#include <iostream>
#include <chrono>
#include <memory>

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::http_nt::client client;
	libgs::http_nt::request_arg arg;

	auto context = client.make_get({"http://baidu.com", arg});

	return 0;
}