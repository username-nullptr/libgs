#include <libgs/core/value.h>
#include <libgs/core/log.h>

int main()
{
#if 1
	libgs::value v0 = "11";
	libgs_log_debug() << v0/*.to_string()*/ << v0.to_int();

	libgs::value v1 = 123;
	libgs_log_debug() << v1 << v1.to_int();

	libgs::value v2 = 3.14;
	libgs_log_debug() << v2 << v2.to_float() << v2.to_int();

	libgs::value v3 = v0;
	libgs_log_debug() << v3 << v3.to_int();

	libgs::value v4 = std::move(v2);
	libgs_log_debug() << v4 << v4.to_float() << v4.to_int();

	libgs::value v5 = "hello";
	libgs_log_debug() << v5 << v5.to_int_or(); /* << v5.to_int() */; // throw !!!
#else
	libgs::wvalue v0 = L"11";
	libgs_log_wdebug() << v0 << v0.to_int();

	libgs::wvalue v1 = 123;
	libgs_log_wdebug() << v1 << v1.to_int();

	libgs::wvalue v2 = 3.14;
	libgs_log_wdebug() << v2 << v2.to_float() << v2.to_int();

	libgs::wvalue v3 = v0;
	libgs_log_wdebug() << v3 << v3.to_int();

	libgs::wvalue v4 = std::move(v2);
	libgs_log_wdebug() << v4 << v4.to_float() << v4.to_int();

	libgs::wvalue v5 = L"hello";
	libgs_log_wdebug() << v5 /* << v5.to_int() */; // throw !!!
#endif
	return 0;
}