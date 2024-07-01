#include <libgs/dbi/cell.h>
#include <spdlog/spdlog.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::dbi::cell cell0("column0");
	spdlog::debug("has_value:", cell0.has_value());
	spdlog::debug("as string:", cell0.to_string_or("NULL"), "\n");

	libgs::dbi::cell cell1("column", "114514");
	spdlog::debug("as string:", cell1.to_string());
	spdlog::debug("as int   :", cell1.to_int());
	spdlog::debug("as blob  :", cell1.blob());
	return 0;
}