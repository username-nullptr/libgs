#include <libgs/core/value.h>
#ifdef _WIN32
# define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#endif //_WIN32
#include <spdlog/spdlog.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);
#if 1
	libgs::value v0 = "11";
	spdlog::debug("{} - {}", v0/*.to_string()*/, v0.to_int());

	libgs::value v1 = 123;
	spdlog::debug("{} - {}", v1, v1.to_int());

	libgs::value v2 = 3.14;
	spdlog::debug("{} - {}", v2, v2.to_float(), v2.to_int());

	libgs::value v3 = v0;
	spdlog::debug("{} - {}", v3, v3.to_int());

	libgs::value v4 = std::move(v2);
	spdlog::debug("{} - {}", v4, v4.to_float(), v4.to_int());

	libgs::value v5 = "hello";
	spdlog::debug("{} - {}", v5, v5.to_int_or()/*, v5.to_int()*/); // throw !!!
#elif defined(_WIN32)
	libgs::wvalue v0 = L"11";
	spdlog::debug(L"{} - {}", v0, v0.to_int());

	libgs::wvalue v1 = 123;
	spdlog::debug(L"{} - {}", v1, v1.to_int());

	libgs::wvalue v2 = 3.14;
	spdlog::debug(L"{} - {}", v2, v2.to_float(), v2.to_int());

	libgs::wvalue v3 = v0;
	spdlog::debug(L"{} - {}", v3, v3.to_int());

	libgs::wvalue v4 = std::move(v2);
	spdlog::debug(L"{} - {}", v4, v4.to_float(), v4.to_int());

	libgs::wvalue v5 = L"hello";
	spdlog::debug(L"{} - {}", v5 /*, v5.to_int() */); // throw !!!
#endif
	return 0;
}