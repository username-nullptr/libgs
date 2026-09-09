// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_STRING_CONTAINER_H
#define LIBGS_CORE_STRING_CONTAINER_H

#include <libgs/core/global.h>
#include <string>

namespace libgs { namespace concepts
{

template <typename T, typename CharT, template<typename,typename...> class Container, typename...Args>
concept str_container_iter =
	std::is_same_v<T, typename Container<std::basic_string<CharT>,Args...>::iterator> or
	std::is_same_v<T, typename Container<std::basic_string<CharT>,Args...>::const_iterator> or
	std::is_same_v<T, typename Container<std::basic_string<CharT>,Args...>::reverse_iterator> or
	std::is_same_v<T, typename Container<std::basic_string<CharT>,Args...>::const_reverse_iterator>;

} //namespace concepts

template <concepts::character CharT, template<typename,typename...> class Container, typename...Args>
class LIBGS_CORE_TAPI basic_string_container : public Container<std::basic_string<CharT>,Args...>
{
public:
	using char_t = CharT;
	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

	using base_t = Container<string_t,Args...>;
	using base_t::base_t;

	static constexpr char_t space = 0x20;

public:
	template <concepts::text_p<CharT> Text = char_t>
	[[nodiscard]] string_t join(const Text &splits = space) const;

	template <concepts::text_p<CharT> Text = char_t>
	[[nodiscard]] string_t join(size_t index, size_t length, const Text &splits = space) const;

	template <concepts::text_p<CharT> Text = char_t>
	[[nodiscard]] string_t join(size_t index, const Text &splits = space) const;

	template <concepts::str_container_iter<CharT,Container,Args...> Iter,
			  concepts::text_p<CharT> Text = char_t>
	[[nodiscard]] static string_t join(Iter begin, Iter end, const Text &splits = space)
	{
		string_t result;
		auto view = strtls::to_view(splits);

		for(auto it=begin; it!=end; ++it)
			result += *it + string_t(view.data(), view.size());

		if(begin != end and not view.empty())
			result.erase(result.size() - view.size(), view.size());
		return result;
	}

	template <concepts::text_p<CharT> Str = char_t>
	[[nodiscard]] static basic_string_container from_string (
		concepts::string_p<char_t> auto &&str, const Str &splits = space,
		bool ignore_empty = true
	);
};

} //namespace libgs
#include <libgs/core/detail/string_container.h>


#endif //LIBGS_CORE_STRING_CONTAINER_H
