#include <libgs/core/args_parser.h>
#include <spdlog/spdlog.h>

int main(int argc, const char *argv[])
{
	spdlog::set_level(spdlog::level::trace);
	libgs::string_list others;

	auto args = libgs::cmdline::args_parser("help:")
		.add_group("-f,--file", "123", "a")
		.add_flag("-a", "234", "b")
		.add_flag("-d", "345", "c")
		.add_flag("qwe", "456", "d")
		.set_version("v0.0.1")
		.set_v("==== v0.0.1 ====")
		.enable_h()
		.parsing(argc, argv, others);

	for(auto &[key,value] : args)
		spdlog::debug("===== : {} - {}", key, value);

	for(auto &arg : others)
		spdlog::debug("other : {}", arg);
	return 0;
}