
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_SYSTEM_DETAIL_CPU_H
#define LIBGS_CORE_SYSTEM_DETAIL_CPU_H

#ifdef _MSC_VER
# include <winnt.h>
#endif

namespace libgs
{

inline void none_instruction() noexcept
{
#ifdef _MSC_VER
	_mm_pause();
#elif defined(__GNUC__)
# if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
	asm volatile("pause" : : : "memory");
# elif defined(__aarch64__) || defined(__arm64__) || defined(__arm__)
	asm volatile("yield" : : : "memory");
# elif defined(__powerpc__) || defined(__ppc__)
	asm volatile("or 0, 0, 0" : : : "memory");
# elif defined(__riscv)
	asm volatile("wfi" : : : "memory");
# else // Unknown
	asm volatile("" : : : "memory");
# endif // CPU Architecture
#endif //_MSC_VER
}

} //namespace libgs


#endif //LIBGS_CORE_SYSTEM_DETAIL_CPU_H
