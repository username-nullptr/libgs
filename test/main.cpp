#include <libgs/core/log.h>
#include <libgs/core/value.h>

using namespace std::chrono;

int main()
{
	libgs::value v = "3312";

	libgs_log_debug("====== {}", v.to_int());

	return 0;
}
