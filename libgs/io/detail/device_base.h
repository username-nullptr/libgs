
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

#ifndef LIBGS_IO_DETAIL_DEVICE_BASE_H
#define LIBGS_IO_DETAIL_DEVICE_BASE_H

namespace libgs::io
{

template <typename CC, concept_execution Exec>
device_base<CC,Exec>::device_base(const executor_type &exec) :
	m_exec(exec),
	m_valid(new std::atomic_bool(true))
{

}

template <typename CC, concept_execution Exec>
device_base<CC,Exec>::~device_base()
{
	*m_valid = false;
}

template <typename CC, concept_execution Exec>
typename device_base<CC,Exec>::executor_type &device_base<CC,Exec>::executor() const
{
	return m_exec;
}

template <typename CC, concept_execution Exec>
const typename device_base<CC,Exec>::derived_type &device_base<CC,Exec>::derived() const
{
	return reinterpret_cast<const derived_type&>(*this);
}

template <typename CC, concept_execution Exec>
typename device_base<CC,Exec>::derived_type &device_base<CC,Exec>::derived()
{
	return reinterpret_cast<derived_type&>(*this);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_DEVICE_BASE_H
