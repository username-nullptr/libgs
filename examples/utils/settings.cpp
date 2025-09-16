#include <libgs/utils/settings.h>
#include <libgs/utils/logger.h>

int main()
{
	auto &settings = libgs::utils::settings::instance();
	settings.load("/home/pi/app/config/config.ini");

	settings.changed.connect([](std::string_view path, const libgs::value &value) {
		libgs_utils_log_info("on changed: {} - {}", path, value);
	});

	settings
	.set("group0/key0", "hello")
	.set("group0/key1", 123)
	.set("group0/key2", libgs::value("hello {} {}", 123, "str"));

	settings
	.set({"group1", "key0"}, "hello")
	.set({"group1", "key1"}, 123)
	.set({"group1", "key2"}, libgs::value("hello {} {}", 123, "str"));

	settings.sync();

	auto value0 = settings.get("group0/key0");

	int value1 = *settings.get("group0/key1").or_else()->get<int>().or_else();
	int value2 = *settings.get("group0/key1").or_else()->get<int>().or_else();

	auto value3 = settings.get<std::string>("group0/key2");
	auto value4 = settings.get("group0/key2").or_else();

	auto value5 = settings.get("group0/key0").or_else("none");

	int value6 = settings.get("group0/key1").or_else(123);
	int value7 = settings.get("group0/key1").or_else(libgs::value(123))->get<int>().or_else(123);

	auto value8 = settings.get("group0/key2").or_else("hello");

	libgs::ignore_unused(value0, value1, value2, value3, value4, value5, value6, value7, value8);

	using namespace std::chrono_literals;
	libgs::post(2s, []{
		libgs::exit();
	});
	return libgs::exec();
}