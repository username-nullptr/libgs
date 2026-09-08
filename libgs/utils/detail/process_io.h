// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_UTILS_DETAIL_PROCESS_IO_H
#define LIBGS_UTILS_DETAIL_PROCESS_IO_H

#include <libgs/core/async_expected.h>

namespace libgs::utils::detail
{

template <typename Token, typename Initiation>
[[nodiscard]] auto initiate_process_io(Initiation initiation, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));

	return asio::async_initiate<token_t,void(error_code,size_t)>(
		std::move(initiation), completion_token
	);
}

inline std::shared_ptr<std::string> copy_process_buffer(const const_buffer &buffer)
{
	auto result = std::make_shared<std::string>();
	if( buffer.size() > 0 )
	{
		result->assign(
			static_cast<const char*>(buffer.data()), buffer.size()
		);
	}
	return result;
}

} //namespace libgs::utils::detail


#endif //LIBGS_UTILS_DETAIL_PROCESS_IO_H
