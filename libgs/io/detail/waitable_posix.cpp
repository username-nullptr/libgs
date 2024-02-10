
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

#ifndef __unix__
# error "This file can only be compiled on unix (non-Apple)."
#else

#include "waitable.h"
#include <poll.h>

namespace libgs::io::detail
{

using duration = std::chrono::milliseconds;

static bool wait(int fd, int event, const std::chrono::milliseconds &ms, error_code &error)
{
	struct pollfd fds;
	fds.fd = fd;
	fds.events = event;

	int tt = ms.count() > 0? ms.count() : -1;
	error = std::make_error_code(static_cast<std::errc>(0));

	for(;;)
	{
		int res = poll(&fds, 1, tt);
		if( res < 0 )
		{
			error = std::make_error_code(static_cast<std::errc>(errno));
			return false;
		}
		else if( res == 0 )
			return false;
		else if( res == 1 and fds.revents == event )
			break;
	}
	return true;
}

bool wait_writeable(int fd, const duration &ms, error_code *error) noexcept
{
	error_code _error;
	bool res = wait(fd, POLLOUT, ms, _error);
	if( error )
		*error = _error;
	return res;
}

bool wait_readable(int fd, const duration &ms, error_code *error) noexcept
{
	error_code _error;
	bool res = wait(fd, POLLIN, ms, _error);
	if( error )
		*error = _error;
	return res;
}

} //namespace libgs::io::detail

#endif //__unix__