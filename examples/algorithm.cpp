#include <libgs/core/algorithm.h>

int main()
{
	auto mt = libgs::get_mime_type("/opt/gdb/aaa");
	libgs_log_debug("mime-type: '{}'.", mt);

	libgs::sha1 sha1("Hello World !!!");
	libgs_log_debug("sha1 base64: '{}'.", sha1.finalize().base64());
	// libgs_log_wdebug(L"sha1 base64: '{}'.", sha1.wbase64());

	auto uuid = libgs::uuid::generate();
	libgs_log_debug("uuid: '{}'.", uuid/*.to_string()*/);

	// auto wuuid = libgs::wuuid::generate();
	// libgs_log_wdebug(L"uuid: '{}'.", wuuid/*.to_string()*/);

	return 0;
}