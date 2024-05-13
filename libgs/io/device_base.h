
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

#ifndef LIBGS_IO_DEVICE_BASE_H
#define LIBGS_IO_DEVICE_BASE_H

#include <libgs/io/types/opt_token.h>

namespace libgs::io
{

template <concept_execution Exec = asio::any_io_executor>
class LIBGS_IO_TAPI device_base
{
	LIBGS_DISABLE_COPY_MOVE(device_base)

public:
	using executor_type = Exec;
	using bool_ptr = std::shared_ptr<bool>;

public:
	explicit device_base(const executor_type &exec);
	virtual ~device_base() = 0;

public:
	virtual void cancel() noexcept = 0;
	executor_type &executor() const;

protected:
	mutable Exec m_exec;
	bool_ptr m_valid;
};

using device = device_base<>;

template <concept_execution Exec = asio::any_io_executor>
using device_base_ptr = std::shared_ptr<device_base<Exec>>;

using device_ptr = device_base_ptr<>;

} //namespace libgs::io
#include <libgs/io/detail/device_base.h>


#endif //LIBGS_IO_DEVICE_BASE_H
