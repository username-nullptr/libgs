#include <libgs.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);

	constexpr const char *buf =
			"GET /hello?a=1&b=2 HTTP/1.1\r\n"
			"Content-Length: 15\r\n"
			"Hello: world\r\n"
			"Cookie: aa=bb; cc=dd\r\n"
			"\r\n"
			"Hello world !!!";

	std::string path;
	libgs::http::method method;
	libgs::http::parameters parameters;
	libgs::http::cookie_values cookies;

	libgs::http::parser parser;
	try {
		parser
		.on_parse_begin([&path, &method, &parameters](std::string_view line_buf, std::error_code &error)
		{
			std::string version;
			auto request_line_parts = libgs::string_list::from_string(line_buf, ' ');
			if( request_line_parts.size() != 3 or not libgs::str_to_upper(request_line_parts[2]).starts_with("HTTP/") )
			{
				error = libgs::http::parser::make_error_code(libgs::http::parse_errno::IRL);
				return version;
			}
			try {
				method = libgs::http::from_method_string(request_line_parts[0]);
			}
			catch(std::exception&)
			{
				error = libgs::http::parser::make_error_code(libgs::http::parse_errno::IHM);
				return version;
			}
			version = request_line_parts[2].substr(5,3);
			auto url_line = libgs::from_percent_encoding(request_line_parts[1]);
			auto pos = url_line.find('?');

			if( pos == std::string::npos )
				path = url_line;
			else
			{
				path = url_line.substr(0, pos);
				auto parameters_string = url_line.substr(pos + 1);

				for(auto &para_str : libgs::string_list::from_string(parameters_string, '&'))
				{
					pos = para_str.find('=');
					if( pos == std::string::npos )
						parameters.emplace(para_str, para_str);
					else
						parameters.emplace(para_str.substr(0, pos), para_str.substr(pos+1));
				}
			}
			if( not path.starts_with("/") )
			{
				error = libgs::http::parser::make_error_code(libgs::http::parse_errno::IHP);
				return version;
			}
			auto n_it = std::unique(path.begin(), path.end(), [](char c0, char c1){
				return c0 == c1 and c0 == '/';
			});
			if( n_it != path.end() )
				path.erase(n_it, path.end());

			if( path.size() > 1 and path.ends_with("/") )
				path.pop_back();
			return version;
		})
		.on_parse_cookie([&cookies](std::string_view line_buf, std::error_code &error)
		{
			auto list = libgs::string_list::from_string(line_buf, ';');
			for(auto &statement : list)
			{
				statement = libgs::str_trimmed(statement);
				auto pos = statement.find('=');

				if( pos == std::string::npos )
				{
					error = libgs::http::parser::make_error_code(libgs::http::parse_errno::IHL);
					return ;
				}
				auto key = libgs::str_trimmed(statement.substr(0,pos));
				auto value = libgs::str_trimmed(statement.substr(pos+1));
				cookies[std::move(key)] = std::move(value);
			}
		});
		bool res = parser.append(buf);
		spdlog::debug("11111: {}", res);

		spdlog::debug("Version:{} - Method:{} - Path:{}",
					  parser.version(), libgs::http::to_method_string(method), path);

		for(auto &[key,value] : parameters)
			spdlog::debug("Parameter: {}: {}", key, value);

		for(auto &[key,value] : parser.headers())
			spdlog::debug("Header: {}: {}", key, value);

		for(auto &[key,value] : cookies)
			spdlog::debug("Cookie: {}: {}", key, value);

		spdlog::debug("partial_body: {}\n", parser.take_body());
	}
	catch(std::exception &ex)
	{
		spdlog::error("EEEEEEEE: {}", ex);
		return -1;
	}
//	return libgs::execution::exec();
	return 0;
}
