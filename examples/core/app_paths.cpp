#include <libgs/core/system/app_utls.h>
#include <iostream>

template <typename T>
void print_result(std::string_view label, const libgs::sys_expected<T> &result)
{
	if(result)
		std::cout << label << ": " << *result << '\n';
	else
		std::cerr << label << " failed: " << result.error().message() << '\n';
}

int main()
{
	print_result("Executable", libgs::app::file_path());
	print_result("Executable directory", libgs::app::dir_path());
	print_result("Working directory", libgs::app::current_directory());
	print_result("Absolute current directory", libgs::app::absolute_path("."));
	print_result("Current user", libgs::app::current_user());
	print_result("Home directory", libgs::app::home_directory());
	return 0;
}
