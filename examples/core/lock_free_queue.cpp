#include <libgs/core/lock_free_queue.h>
#include <libgs/core/log.h>

using namespace std::chrono_literals;

static libgs::lock_free_queue<int> queue;

void producer_0()
{
	libgs::sleep_for(1ms);
	for(int i=0; i<200; i++)
		queue.enqueue(i);
}

void producer_1()
{
	libgs::sleep_for(1ms);
	for(int i=200; i<400; i++)
		queue.enqueue(i);
}

void producer_2()
{
	libgs::sleep_for(1ms);
	for(int i=400; i<600; i++)
		queue.enqueue(i);
}

void consumer_0()
{
	for(int i=0;; i++)
	{
		auto op = queue.dequeue();
		if( op )
			std::cout << i << " : " << *op << std::endl;
		else
			break;
	}
}

int main()
{
	std::thread t0(producer_0);
	std::thread t1(producer_1);
	std::thread t2(producer_2);
	t0.join();
	t1.join();
	t2.join();

	std::thread t3(consumer_0);
	t3.join();

	return 0;
}