
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
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
#include <libgs/core/value.h>

namespace libgs::utils
{

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

	template <concepts::match_sched<Exec> Exec0>
	basic_process(Exec0 &&exec, string_t cmd = {}, args_t args = {});

	template <concepts::match_sched<Exec> Exec0, typename...Args>
	basic_process(Exec0 &&exec, string_t cmd, Args&&...args)
		requires concepts::formatter<char_t,Args...>;

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
	void detach() noexcept;
	void cancel() noexcept;

public:
	template <typename Token>
	static constexpr bool task_token_v = concepts::time_p<Token> or
		concepts::tf_opt_token<Token,error_code,int>;

	template <typename Token>
	static constexpr bool join_token_v =
		not is_detached_v<Token> and task_token_v<Token>;

	template <typename Token = std::chrono::nanoseconds>
	auto join(Token &&token = {}) noexcept
		requires join_token_v<Token>;

	template <concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(const const_buffer &buf, Token &&token = {}) noexcept;

	template <concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto read(const mutable_buffer &buf, Token &&token = {}) noexcept;

	template <concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto read_stderr(const mutable_buffer &buf, Token &&token = {}) noexcept;

public:
	template <typename Token = std::chrono::nanoseconds>
	auto run(const string_t &cmd, const args_t &args, Token &&token = {}) noexcept
		requires task_token_v<Token>;

	template <typename Token = std::chrono::nanoseconds>
	auto run(const string_t &cmd, Token &&token = {}) noexcept
		requires task_token_v<Token>;

	template <typename Token = std::chrono::nanoseconds>
	auto run(Token &&token = {}) noexcept
		requires task_token_v<Token>;

public:
	void set_work_path(path_t path) noexcept;
	void setenv(std::string_view key, libgs::value value) noexcept;
	void unsetenv(std::string_view key) noexcept;

public:
	[[nodiscard]] state_t state() const noexcept;
	[[nodiscard]] int exit_code() const noexcept;

	[[nodiscard]] uint64_t pid() const noexcept;
	[[nodiscard]] executor_t get_executor() const noexcept;

public:
	template <concepts::opt_token<error_code> Token = use_sync_t>
	static auto exec(const string_t &cmd, const args_t &args, Token &&token = {}) noexcept;

	template <concepts::opt_token<error_code> Token = use_sync_t>
	static auto exec(const string_t &cmd, Token &&token = {}) noexcept;

	template <concepts::match_sched<Exec> Exec0, concepts::opt_token<error_code> Token = use_sync_t>
	static auto exec(Exec0 &&exec, const string_t &cmd, const args_t &args, Token &&token = {}) noexcept;

	template <concepts::match_sched<Exec> Exec0, concepts::opt_token<error_code> Token = use_sync_t>
	static auto exec(Exec0 &&exec, const string_t &cmd, Token &&token = {}) noexcept;

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