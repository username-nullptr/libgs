#include <libgs/core/lock_free_queue.h>
#include <libgs/core/execution.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

// 0: linked list  >0: circular buffer
static constexpr size_t capacity = 0;

static libgs::lock_free_queue<int,capacity> queue;

void producer_0()
{
	libgs::sleep_for(1000us);
	for(int i=0; i<200; i++)
	{
		queue.enqueue(i);
		libgs::sleep_for(70us);
	}
}

void producer_1()
{
	libgs::sleep_for(950us);
	for(int i=200; i<400; i++)
	{
		queue.enqueue(i);
		libgs::sleep_for(140us);
	}
}

void producer_2()
{
	libgs::sleep_for(900us);
	for(int i=400; i<600; i++)
	{
		queue.enqueue(i);
		libgs::sleep_for(100us);
	}
}

static std::atomic_bool g_run {true};
static std::atomic_size_t g_count {0};

void consumer_0()
{
	for(;;)
	{
		auto size = queue.size();
		auto op = queue.dequeue();
		if( op )
			spdlog::debug("c0 : {} | {} : {}", size, g_count++, *op);
		else if( not g_run )
			break;
	}
}

void consumer_1()
{
	for(;;)
	{
		auto size = queue.size();
		auto op = queue.dequeue();
		if( op )
			spdlog::debug("c1 : {} | {} : {}", size, g_count++, *op);
		else if( not g_run )
			break;
	}
}

int main()
{
	spdlog::set_level(spdlog::level::trace);

	std::thread t3(consumer_0);
	std::thread t4(consumer_1);

	std::thread t0(producer_0);
	std::thread t1(producer_1);
	std::thread t2(producer_2);

	t0.join();
	t1.join();
	t2.join();

	g_run = false;
	t3.join();
	t4.join();
	return 0;
}