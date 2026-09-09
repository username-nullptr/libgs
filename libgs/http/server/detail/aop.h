// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_SERVER_DETAIL_AOP_H
#define LIBGS_HTTP_SERVER_DETAIL_AOP_H

namespace libgs::http
{

template <core_concepts::exec Exec>
basic_aop<Exec>::~basic_aop() = default;

template <core_concepts::exec Exec>
awaitable<bool> basic_aop<Exec>::before(context_t &context)
{
	ignore_unused(context);
	co_return false;
}

template <core_concepts::exec Exec>
awaitable<bool> basic_aop<Exec>::after(context_t &context)
{
	ignore_unused(context);
	co_return false;
}

template <core_concepts::exec Exec>
bool basic_aop<Exec>::exception(context_t &context, const std::exception &ex)
{
	ignore_unused(context, ex);
	return false;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_AOP_H
