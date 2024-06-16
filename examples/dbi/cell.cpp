#include <libgs/core/log.h>
#include <libgs/dbi/cell.h>

int main()
{
	libgs::dbi::cell cell0("column0");
	libgs_log_debug() << "has_value:" << cell0.has_value();
	libgs_log_debug() << "as string:" << cell0.to_string_or("NULL") << "\n";

	libgs::dbi::cell cell1("column", "114514");
	libgs_log_debug() << "as string:" << cell1.to_string();
	libgs_log_debug() << "as int   :" << cell1.to_int();
	libgs_log_debug() << "as blob  :" << cell1.blob();
	return 0;
}