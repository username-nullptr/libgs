#include <libgs/core/execution.h>
#include <spdlog/spdlog.h>

int main()
{
	using namespace std::chrono_literals;
	asio::io_context ioc;
	/*
	 * If the current thread context is the same as
	 * the target thread context, execute immediately;
	 * otherwise, the task will be pushed into
	 * the event queue of the target context.
	 */
	libgs::dispatch(/* ioc, */ []{
		spdlog::info("dispatch");
	});
	/*
	 * Push the task to the event queue of the target context
	 */
	libgs::post(/* ioc, */ []{
		spdlog::info("post");
	});
	/*
	 * Start an internal timer. Once the time is up,
	 * the task will be executed by the target context.
	 */
	libgs::post(2s, /* ioc, */ []{
		spdlog::info("2s post");
	});
	libgs::post(std::chrono::system_clock::now() + 3s, /* ioc, */ []{
		spdlog::info("now + 3s post");
	});
	/*
	 * Start a loop timer, return the timer object,
	 * and when it is destructed, the timer will stop.
	 * If the last parameter is true,
	 * the task will be executed immediately.
	 */
	libgs::start_timer(1s, /* ioc, */ []{
		spdlog::info("timer");
	}
	/*, true*/);

	// Exit in 10 seconds.
	libgs::post(10s, []{
		libgs::exit(3);
	});
	return libgs::exec();
}