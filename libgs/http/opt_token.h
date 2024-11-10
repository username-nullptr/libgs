
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

#ifndef LIBGS_HTTP_OPT_TOKEN_H
#define LIBGS_HTTP_OPT_TOKEN_H

#include <libgs/http/types.h>
#include <fstream>

namespace libgs::http
{

template <typename...Args>
using callback_t = std::function<void(Args...)>;

class LIBGS_HTTP_API op_base
{
public:
    op_base() = default;
    op_base(error_code &error);
    error_code *error = nullptr;
};

class LIBGS_HTTP_API read_op : public op_base
{
public:
    read_op(mutable_buffer buf, error_code &error);
    read_op(mutable_buffer buf);
    mutable_buffer buf;
};

class LIBGS_HTTP_API write_op : public op_base
{
public:
    write_op(const_buffer buf, error_code &error);
    write_op(const_buffer buf);
    write_op();
    const_buffer buf;
};

template <typename FS>
class LIBGS_HTTP_TAPI file_op_base : public op_base
{
public:
    using fstream_t = FS;
	using pos_t = typename fstream_t::pos_type;
    fstream_t *fs;

    file_op_base(fstream_t &&fs, error_code &error);
    file_op_base(fstream_t &fs, error_code &error);
    file_op_base(fstream_t &fs);

    template <typename...Args>
    file_op_base(error_code &error, Args&&...args) requires
        core_concepts::constructible<fstream_t,Args&&...>;

    template <typename...Args>
    file_op_base(Args&&...args) requires
		core_concepts::constructible<fstream_t, Args&&...>;

protected:
    bool ext;
};

template <typename FS>
class LIBGS_HTTP_TAPI req_file_op_base : public file_op_base<FS>
{
	using base_t = file_op_base<FS>;

public:
	using pos_t = typename base_t::pos_t;
	using fstream_t = typename base_t::fstream_t;
	using base_t::file_op_base;

	struct range
	{
		pos_t begin = 0;
		pos_t total = 0;
	}
	rng;

    req_file_op_base(fstream_t &&fs, range rng, error_code &error);
    req_file_op_base(fstream_t &fs, range rng, error_code &error);
    req_file_op_base(fstream_t &fs, range rng, pos_t total);

    template <typename...Args>
    req_file_op_base(error_code &error, range rng, Args&&...args) requires
        core_concepts::constructible<fstream_t,Args&&...>;

    template <typename...Args>
    req_file_op_base(pos_t begin, range rng, Args&&...args) requires
		core_concepts::constructible<fstream_t, Args&&...>;
};

template <core_concepts::char_type CharT>
using basic_req_file_op = req_file_op_base<std::basic_fstream<CharT>>;

using req_file_op = basic_req_file_op<char>;
using basic_req_file_op = basic_req_file_op<wchar_t>;

template <typename FS>
class LIBGS_HTTP_TAPI resp_file_op_base : public file_op_base<FS>
{
	using base_t = file_op_base<FS>;

public:
	using pos_t = typename base_t::pos_t;
	using fstream_t = typename base_t::fstream_t;
	using base_t::file_op_base;

	struct range
	{
		pos_t begin = 0;
		pos_t end = 0;
	};
	using ranges_t = std::vector<range>;
	ranges_t rngs;

    resp_file_op_base(fstream_t &&fs, ranges_t rngs, error_code &error);
    resp_file_op_base(fstream_t &&fs, range rng, error_code &error);

    resp_file_op_base(fstream_t &fs, ranges_t rngs, error_code &error);
    resp_file_op_base(fstream_t &fs, range rng, error_code &error);

    resp_file_op_base(fstream_t &fs, ranges_t rngs);
    resp_file_op_base(fstream_t &fs, range rng);

    template <typename...Args>
    resp_file_op_base(error_code &error, ranges_t rngs, Args&&...args) requires
        core_concepts::constructible<fstream_t,Args&&...>;

    template <typename...Args>
    resp_file_op_base(error_code &error, range rng, Args&&...args) requires
        core_concepts::constructible<fstream_t,Args&&...>;

    template <typename...Args>
    resp_file_op_base(pos_t begin, ranges_t rngs, Args&&...args) requires
		core_concepts::constructible<fstream_t, Args&&...>;

    template <typename...Args>
    resp_file_op_base(pos_t begin, range rng, Args&&...args) requires
		core_concepts::constructible<fstream_t, Args&&...>;
};

template <core_concepts::char_type CharT>
using basic_resp_file_op = resp_file_op_base<std::basic_fstream<CharT>>;

using resp_file_op = basic_resp_file_op<char>;
using basic_resp_file_op = basic_resp_file_op<wchar_t>;

namespace opreators
{

LIBGS_HTTP_API read_op operator| (
	mutable_buffer buf, error_code &error
);
LIBGS_HTTP_API read_op operator| (
	error_code &error, mutable_buffer buf
);

LIBGS_HTTP_API write_op operator| (
	const_buffer buf, error_code &error
);
LIBGS_HTTP_API write_op operator| (
	error_code &error, const_buffer buf
);

template <typename FS>
LIBGS_HTTP_TAPI file_op_base<FS> operator| (
	FS &&fs, error_code &error
);
template <typename FS>
LIBGS_HTTP_TAPI file_op_base<FS> operator| (
	error_code &error, FS &&fs
);

template <typename FS>
LIBGS_HTTP_TAPI req_file_op_base<FS> operator| (
	file_op_base<FS> fs, typename req_file_op_base<FS>::range rng
);
template <typename FS>
LIBGS_HTTP_TAPI req_file_op_base<FS> operator| (
	typename req_file_op_base<FS>::range rng, file_op_base<FS> fs
);

template <typename FS>
LIBGS_HTTP_TAPI resp_file_op_base<FS> operator| (
	file_op_base<FS> fs, typename resp_file_op_base<FS>::range rng
);
template <typename FS>
LIBGS_HTTP_TAPI resp_file_op_base<FS> operator| (
	typename resp_file_op_base<FS>::range rng, file_op_base<FS> fs
);

template <typename FS>
LIBGS_HTTP_TAPI resp_file_op_base<FS> operator| (
	file_op_base<FS> fs, typename resp_file_op_base<FS>::ranges_t rngs
);
template <typename FS>
LIBGS_HTTP_TAPI resp_file_op_base<FS> operator| (
	typename resp_file_op_base<FS>::ranges_t rngs, file_op_base<FS> fs
);

template <typename FS>
LIBGS_HTTP_TAPI resp_file_op_base<FS> &operator| (
	resp_file_op_base<FS> &fs, typename resp_file_op_base<FS>::range rng
);
template <typename FS>
LIBGS_HTTP_TAPI resp_file_op_base<FS> &operator| (
	typename resp_file_op_base<FS>::range rng, resp_file_op_base<FS> &fs
);

template <typename FS>
LIBGS_HTTP_TAPI resp_file_op_base<FS> &operator| (
	resp_file_op_base<FS> &fs, typename resp_file_op_base<FS>::ranges_t rngs
);
template <typename FS>
LIBGS_HTTP_TAPI resp_file_op_base<FS> &operator| (
	typename resp_file_op_base<FS>::ranges_t rngs, resp_file_op_base<FS> &fs
);

}} //namespace libgs::http::opreators
#include <libgs/http/detail/opt_token.h>


#endif //LIBGS_HTTP_OPT_TOKEN_H
