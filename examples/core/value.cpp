// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/core/value.h>

#include <iostream>

int main()
{
	libgs::value text = "42";
	libgs::value integer = 42;
	libgs::value decimal = 3.5;
	libgs::value formatted("answer = {}", integer);

	std::cout << text.to_string() << " as int: "
		<< text.to_int().value_or(0) << '\n';

	std::cout << integer.to_string() << " as string: "
		<< integer.to_string() << '\n';

	std::cout << decimal.to_string() << " as double: "
		<< decimal.to_double().value_or(0) << '\n';

	std::cout << formatted.to_string() << '\n';

	libgs::value invalid = "not-a-number";
	auto conversion = invalid.get<int>();

	std::cout << "Invalid conversion has value: "
		<< std::boolalpha << static_cast<bool>(conversion) << '\n';

	return 0;
}
