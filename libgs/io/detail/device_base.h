
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

template <typename Derived, concept_execution Exec>
device_base<Derived,Exec>::device_base(const executor_t &exec) :
	m_exec(exec),
	m_valid(new std::atomic_bool(true))
{

}

template <typename Derived, concept_execution Exec>
device_base<Derived,Exec>::~device_base()
{
	*m_valid = false;
}

template <typename Derived, concept_execution Exec>
template <concept_execution Exec0>
device_base<Derived,Exec>::device_base(device_base<Derived,Exec0> &&other) noexcept :
	m_exec(other.m_exec),
	m_valid(other.m_valid)
{

}

template <typename Derived, concept_execution Exec>
template <concept_execution Exec0>
device_base<Derived,Exec> &device_base<Derived,Exec>::operator=(device_base<Derived,Exec0> &&other) noexcept
{
	m_exec = other.m_exec;
	m_valid = other.m_valid;
	return *this;
}

template <typename Derived, concept_execution Exec>
typename device_base<Derived,Exec>::executor_t &device_base<Derived,Exec>::executor() const
{
	return m_exec;
}

template <typename Derived, concept_execution Exec>
const typename device_base<Derived,Exec>::derived_t &device_base<Derived,Exec>::derived() const
{
	return reinterpret_cast<const derived_t&>(*this);
}

template <typename Derived, concept_execution Exec>
typename device_base<Derived,Exec>::derived_t &device_base<Derived,Exec>::derived()
{
	return reinterpret_cast<derived_t&>(*this);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_DEVICE_BASE_H
