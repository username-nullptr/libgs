#include <libgs/utils/process.h>

#include <chrono>
#include <iostream>
#include <string>

using namespace std::chrono_literals;

namespace
{

using process_t = libgs::utils::process;

struct command_t
{
	std::string file;
};

[[nodiscard]] command_t exit_command(int code)
{
#if defined(_WIN32)
	return {std::format("cmd /C \"exit /B {}\"", code)};
#else
	return {std::format("/bin/sh -c \"exit {}\"", code)};
#endif
}

[[nodiscard]] command_t stdout_command()
{
#if defined(_WIN32)
	return {"cmd /C \"echo libgs_process_stdout\""};
#else
	return {"/bin/sh -c \"printf libgs_process_stdout\""};
#endif
}

#if defined(_WIN32)
[[nodiscard]] command_t stderr_command()
{
	return {"cmd /C \"echo libgs_process_stderr 1>&2\""};
}
#endif

[[nodiscard]] command_t env_command()
{
#if defined(_WIN32)
	return {"cmd /C \"echo %LIBGS_PROCESS_TEST_ENV%\""};
#else
	return {"/usr/bin/printenv LIBGS_PROCESS_TEST_ENV"};
#endif
}

bool expect(bool expr, std::string_view message)
{
	if( expr )
		return true;

	std::cerr << "FAILED: " << message << '\n';
	return false;
}

bool expect_success(const auto &expected, std::string_view message)
{
	if( expected )
		return true;

	std::cerr << "FAILED: " << message << ": "
		<< expected.error().message() << '\n';
	return false;
}

bool test_exit_code()
{
	auto cmd = exit_command(7);
	process_t proc;

	auto expected = proc.run(cmd.file);
	if( not expect_success(expected, "process run should succeed") )
		return false;

	return expect(*expected == 7, "process exit code should be preserved");
}

bool test_stdout_read()
{
	auto cmd = stdout_command();
	process_t proc;

	auto started = proc.start(cmd.file);
	if( not expect_success(started, "stdout process should start") )
		return false;

	char buf[256] {};
	auto read = proc.read(asio::buffer(buf));
	if( not expect_success(read, "stdout should be readable") )
		return false;

	auto joined = proc.join(3s);
	if( not expect_success(joined, "stdout process should join") )
		return false;

	std::string output(buf, *read);
	return expect (
		output.find("libgs_process_stdout") != std::string::npos,
		"stdout should contain expected text"
	);
}

#if defined(_WIN32)
bool test_stderr_read()
{
	auto cmd = stderr_command();
	process_t proc;

	auto started = proc.start(cmd.file);
	if( not expect_success(started, "stderr process should start") )
		return false;

	char buf[256] {};
	auto read = proc.read_stderr(asio::buffer(buf));
	if( not expect_success(read, "stderr should be readable") )
		return false;

	auto joined = proc.join(3s);
	if( not expect_success(joined, "stderr process should join") )
		return false;

	std::string output(buf, *read);
	return expect (
		output.find("libgs_process_stderr") != std::string::npos,
		"stderr should contain expected text"
	);
}
#endif

bool test_environment()
{
	auto cmd = env_command();
	process_t proc;
	proc.setenv("LIBGS_PROCESS_TEST_ENV", "libgs_process_env");

	auto started = proc.start(cmd.file);
	if( not expect_success(started, "env process should start") )
		return false;

	char buf[256] {};
	auto read = proc.read(asio::buffer(buf));
	if( not expect_success(read, "env stdout should be readable") )
		return false;

	auto joined = proc.join(3s);
	if( not expect_success(joined, "env process should join") )
		return false;

	std::string output(buf, *read);
	return expect (
		output.find("libgs_process_env") != std::string::npos,
		"environment variable should be visible to child process"
	);
}

} // namespace

int main()
{
	bool ok = true;

	ok = test_exit_code() and ok;
	ok = test_stdout_read() and ok;

#if defined(_WIN32)
	ok = test_stderr_read() and ok;
#endif

	ok = test_environment() and ok;
	return ok ? 0 : 1;
}
