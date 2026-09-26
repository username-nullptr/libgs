// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_UTILS_ERROR_CODE_ADAPTER_H
#define LIBGS_CORE_UTILS_ERROR_CODE_ADAPTER_H

#include <libgs/core/utils/token_concepts.h>
#include <libgs/core/cxx/cplusplus.h>

namespace libgs
{

template <concepts::error_code_token Error>
class LIBGS_CORE_TAPI error_code_adapter
{
	LIBGS_DISABLE_COPY_MOVE(error_code_adapter)

public:
	explicit error_code_adapter(Error target) noexcept;
	~error_code_adapter();

	[[nodiscard]] error_code &get() noexcept;

private:
	Error m_target;
	error_code m_error;
};

template <typename Error>
[[nodiscard]] LIBGS_CORE_TAPI auto adapt_error_code(Error &error) noexcept
	requires is_error_code_token_v<Error&>;

} //namespace libgs
#include <libgs/core/utils/detail/error_code_adapter.h>


#endif //LIBGS_CORE_UTILS_ERROR_CODE_ADAPTER_H
