
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

#ifndef LIBGS_CORE_LOG_H
#define LIBGS_CORE_LOG_H

#include <libgs/core/log/logger.h>

#define libgs_log_debug    libgs::log::logger(__FILE__, __FUNCTION__, __LINE__).debug
#define libgs_log_info     libgs::log::logger(__FILE__, __FUNCTION__, __LINE__).info
#define libgs_log_warning  libgs::log::logger(__FILE__, __FUNCTION__, __LINE__).warning
#define libgs_log_error    libgs::log::logger(__FILE__, __FUNCTION__, __LINE__).error
#define libgs_log_fatal    libgs::log::logger(__FILE__, __FUNCTION__, __LINE__).fatal

#define libgs_custom_log_debug    libgs::log::logger(__FILE__, __FUNCTION__, __LINE__).cdebug
#define libgs_custom_log_info     libgs::log::logger(__FILE__, __FUNCTION__, __LINE__).cinfo
#define libgs_custom_log_warning  libgs::log::logger(__FILE__, __FUNCTION__, __LINE__).cwarning
#define libgs_custom_log_error    libgs::log::logger(__FILE__, __FUNCTION__, __LINE__).cerror
#define libgs_custom_log_fatal    libgs::log::logger(__FILE__, __FUNCTION__, __LINE__).cfatal

#define libgs_log_wdebug    libgs::log::wlogger(__FILE__, __FUNCTION__, __LINE__).debug
#define libgs_log_winfo     libgs::log::wlogger(__FILE__, __FUNCTION__, __LINE__).info
#define libgs_log_wwarning  libgs::log::wlogger(__FILE__, __FUNCTION__, __LINE__).warning
#define libgs_log_werror    libgs::log::wlogger(__FILE__, __FUNCTION__, __LINE__).error
#define libgs_log_wfatal    libgs::log::wlogger(__FILE__, __FUNCTION__, __LINE__).fatal

#define libgs_custom_log_wdebug    libgs::log::wlogger(__FILE__, __FUNCTION__, __LINE__).cdebug
#define libgs_custom_log_winfo     libgs::log::wlogger(__FILE__, __FUNCTION__, __LINE__).cinfo
#define libgs_custom_log_wwarning  libgs::log::wlogger(__FILE__, __FUNCTION__, __LINE__).cwarning
#define libgs_custom_log_werror    libgs::log::wlogger(__FILE__, __FUNCTION__, __LINE__).cerror
#define libgs_custom_log_wfatal    libgs::log::wlogger(__FILE__, __FUNCTION__, __LINE__).cfatal


#endif //LIBGS_CORE_LOG_H
