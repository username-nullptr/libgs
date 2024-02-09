
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

#ifndef LIBGS_CORE_DETAIL_FLAGS_H
#define LIBGS_CORE_DETAIL_FLAGS_H

namespace libgs
{

template <flag_template Enum>
constexpr inline flags<Enum>::flags(Enum f) noexcept :
	m_value(f)
{

}

template <flag_template Enum>
constexpr inline flags<Enum>::flags(const flags &other) :
	m_value(other.m_value)
{

}

template <flag_template Enum>
constexpr inline flags<Enum>::flags(std::initializer_list<Enum> flags) noexcept :
	m_value(initializer_list_helper(flags.begin(), flags.end()))
{

}

template <flag_template Enum>
inline flags<Enum> &flags<Enum>::operator=(const flags<Enum> &other)
{
	m_value = other.m_value;
	return *this;
}

template <flag_template Enum>
template <flag_number Int>
const inline flags<Enum> &flags<Enum>::operator&=(Int mask) noexcept
{
	m_value &= static_cast<uint32_t>(mask);
	return *this;
}

template <flag_template Enum>
const inline flags<Enum> &flags<Enum>::operator&=(Enum mask) noexcept
{
	m_value &= mask;
	return *this;
}

template <flag_template Enum>
const inline flags<Enum> &flags<Enum>::operator|=(flags f) noexcept
{
	m_value |= f.m_value;
	return *this;
}

template <flag_template Enum>
const inline flags<Enum> &flags<Enum>::operator|=(Enum f) noexcept
{
	m_value |= f;
	return *this;
}

template <flag_template Enum>
const inline flags<Enum> &flags<Enum>::operator^=(flags<Enum> f) noexcept
{
	m_value ^= f.m_value;
	return *this;
}

template <flag_template Enum>
const inline flags<Enum> &flags<Enum>::operator^=(Enum f) noexcept
{
	m_value ^= f;
	return *this;
}

template <flag_template Enum>
template <flag_number Int>
constexpr inline flags<Enum> flags<Enum>::operator&(Int mask) const noexcept
{
	return flags<Enum>( static_cast<Enum>(m_value & static_cast<uint32_t>(mask)));
}

template <flag_template Enum>
constexpr inline flags<Enum> flags<Enum>::operator&(Enum f) const noexcept
{
	return flags<Enum>( static_cast<Enum>(m_value & f) );
}

template <flag_template Enum>
constexpr inline flags<Enum> flags<Enum>::operator|(flags<Enum> f) const noexcept
{
	return flags<Enum>( static_cast<Enum>(m_value | f.m_value) );
}

template <flag_template Enum>
constexpr inline flags<Enum> flags<Enum>::operator|(Enum f) const noexcept
{
	return flags<Enum>( static_cast<Enum>(m_value | f) );
}

template <flag_template Enum>
constexpr inline flags<Enum> flags<Enum>::operator^(flags<Enum> f) const noexcept
{
	return flags<Enum>( static_cast<Enum>(m_value ^ f.m_value) );
}

template <flag_template Enum>
constexpr inline flags<Enum> flags<Enum>::operator^(Enum f) const noexcept
{
	return flags<Enum>( static_cast<Enum>(m_value ^ f) );
}

template <flag_template Enum>
constexpr inline flags<Enum> flags<Enum>::operator~() const noexcept
{
	return flags<Enum>( static_cast<Enum>(~m_value) );
}

template <flag_template Enum>
constexpr inline bool flags<Enum>::operator!() const noexcept
{
	return not m_value;
}

template <flag_template Enum>
constexpr inline flags<Enum>::operator int() const noexcept
{
	return m_value;
}

template <flag_template Enum>
constexpr inline bool flags<Enum>::testFlag(Enum f) const noexcept
{
	return (m_value & f) == f and (f != 0 or m_value == f);
}

template <flag_template Enum>
constexpr inline flags<Enum> &flags<Enum>::setFlag(Enum f, bool on) const noexcept
{
	return on ? (*this |= f) : (*this &= ~f);
}

template <flag_template Enum>
constexpr inline int flags<Enum>::initializer_list_helper(iterator it, iterator end) noexcept
{
	return (it == end ? 0 : (*it | initializer_list_helper(it + 1, end)));
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_FLAGS_H
