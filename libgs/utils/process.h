
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025-2026 Xiaoqiang <username_nullptr@163.com>                    *
*                                                                                   *
*   This file is part of LIBGS                                                      *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software and associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*   copies of the Software, and to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef LIBGS_UTILS_PROCESS_H
#define LIBGS_UTILS_PROCESS_H

#include <libgs/utils/global.h>
#include <libgs/core/execution.h>
#include <libgs/core/async_expected.h>
#include <libgs/core/value.h>

namespace libgs::utils
{

using pid_t = uint64_t;

enum class process_state {
	idle, running, exited, crashed
};

template <concepts::character CharT, concepts::exec Exec = asio::any_io_executor>
class LIBGS_UTILS_TAPI basic_process
{
	LIBGS_DISABLE_COPY(basic_process)

public:
	using char_t = CharT;
	using executor_t = Exec;
	using state_t = process_state;

	using string_t = std::basic_string<char_t>;
	using value_t = libgs::basic_value<char_t>;

	using args_t = std::vector<value_t>;
	using path_t = std::filesystem::path;

public:
	explicit basic_process(string_t cmd = {}, args_t args = {})
		requires concepts::match_sched<io_executor_t,Exec>;

	template <typename...Args>
	basic_process(string_t cmd, Args&&...args) requires
		concepts::match_sched<io_executor_t,Exec> and
		concepts::formatter<char_t,Args...>;

	template <typename Scheduler>
	basic_process(Scheduler &&exec, string_t cmd = {}, args_t args = {}) requires
		(not std::same_as<std::remove_cvref_t<Scheduler>,basic_process>) and
		concepts::match_sched<Scheduler,Exec>;

	template <typename Scheduler, typename...Args>
	basic_process(Scheduler &&exec, string_t cmd, Args&&...args) requires
		(not std::same_as<std::remove_cvref_t<Scheduler>,basic_process>) and
		concepts::match_sched<Scheduler,Exec> and
		concepts::formatter<char_t,Args...>;

	basic_process(basic_process &&other) noexcept;
	basic_process &operator=(basic_process &&other) noexcept;
	~basic_process();

public:
	template <typename...Args>
	sys_expected<> start(const string_t &cmd, Args&&...args) noexcept
		requires concepts::formatter<char_t,Args...>;

	sys_expected<> start (
		const string_t &cmd = {}, const args_t &args = {}
	) noexcept;

	void terminate() noexcept;
	void kill() noexcept;
	void detach();
	void cancel() noexcept;

public:
	template <typename Token, typename...Value>
	static constexpr bool task_token_v =
		concepts::tf_opt_token<Token,error_code,Value...>;

	template <typename Token, typename...Value>
	static constexpr bool dis_detach_token_v =
		task_token_v<Token,Value...> and not is_detached_v<token_unbound_t<Token>>;

public:
	template <typename Token = use_sync_t>
	auto join(Token &&token = {})
		requires dis_detach_token_v<Token,int> or
		concepts::time_p<Token>;

	template <concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(const const_buffer &buf, Token &&token = {});

	template <typename Token = use_sync_t>
	auto read(const mutable_buffer &buf, Token &&token = {}) requires
		dis_detach_token_v<Token,size_t>;

	template <concepts::buffer Buffer, typename Token = use_sync_t>
	auto read(Token &&token = {}) requires
		dis_detach_token_v<Token,Buffer>;

	template <typename Token = use_sync_t>
	auto read(Token &&token = {}) requires
		dis_detach_token_v<Token,std::vector<std::byte>>;

	template <typename Token = use_sync_t>
	auto read_stderr(const mutable_buffer &buf, Token &&token = {}) requires
		dis_detach_token_v<Token,size_t>;

	template <concepts::buffer Buffer, typename Token = use_sync_t>
	auto read_stderr(Token &&token = {}) requires
		dis_detach_token_v<Token,Buffer>;

	template <typename Token = use_sync_t>
	auto read_stderr(Token &&token = {}) requires
		dis_detach_token_v<Token,std::vector<std::byte>>;

public:
	template <typename Token = use_sync_t>
	auto run(const string_t &cmd, const args_t &args, Token &&token = {})
		requires task_token_v<Token,int>;

	template <typename Token = use_sync_t>
	auto run(const string_t &cmd, Token &&token = {})
		requires task_token_v<Token,int>;

	template <typename Token = use_sync_t>
	auto run(Token &&token = {})
		requires task_token_v<Token,int>;

public:
	void set_work_path(path_t path) noexcept;
	void setenv(std::string_view key, libgs::value value) noexcept;
	void unsetenv(std::string_view key) noexcept;

public:
	[[nodiscard]] state_t state() const noexcept;
	[[nodiscard]] int exit_code() const noexcept;

	[[nodiscard]] pid_t pid() const noexcept;
	[[nodiscard]] bool joinable() const noexcept;
	[[nodiscard]] executor_t get_executor() const noexcept;

public:
	template <typename Token>
	static constexpr bool exec_token_v =
		task_token_v<Token,int> or concepts::time_p<Token>;

	template <typename Token = use_sync_t>
	static auto exec(const string_t &cmd, const args_t &args, Token &&token = {})
		requires exec_token_v<Token>;

	template <typename Token = use_sync_t>
	static auto exec(const string_t &cmd, Token &&token = {})
		requires exec_token_v<Token>;

	template <concepts::match_sched<Exec> Exec0, typename Token = use_sync_t>
	static auto exec(Exec0 &&exec, const string_t &cmd, const args_t &args, Token &&token = {})
		requires exec_token_v<Token>;

	template <concepts::match_sched<Exec> Exec0, typename Token = use_sync_t>
	static auto exec(Exec0 &&exec, const string_t &cmd, Token &&token = {})
		requires exec_token_v<Token>;

public:
	[[nodiscard]] static sys_expected<pid_t> self_pid() noexcept;
	static sys_expected<> terminate(pid_t pid) noexcept;
	static sys_expected<> kill(pid_t pid) noexcept;
	/*
	 * @path lock file path, default to home directory.
	 * @return existing pid or self pid.
	 */
	[[nodiscard]] static sys_expected<pid_t> set_single(const path_t &path, std::string_view key);
	[[nodiscard]] static sys_expected<pid_t> set_single(std::string_view key);

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

using process  = basic_process<char   >;
using wprocess = basic_process<wchar_t>;

// using u8process  = basic_process<char8_t >;
// using u16process = basic_process<char16_t>;
// using u32process = basic_process<char32_t>;

} //namespace libgs::utils
#include <libgs/utils/detail/process.h>


#endif //LIBGS_UTILS_PROCESS_H
