#include "modules.h"
#include <libgs/core/execution.h>
#include <ranges>
#include <map>

namespace libgs
{

enum variant_type
{
	func0_e, future_func0_e, await_func0_e,
	func1_e, future_func1_e, await_func1_e
};

using func_obj_t = modules::func_obj_t;
using level_t = modules::level_t;

using func_vector_t = std::vector<func_obj_t>;
using func_map_t = std::map<level_t, func_vector_t>;

static func_map_t &init_map()
{
	static func_map_t map;
	return map;
}

void modules::impl::reg_init(func_obj_t func, level_t level)
{
	auto res = [&func]() -> bool
	{
		if( func.index() == func0_e )
			return std::get<func0_t>(func) != nullptr;

		else if( func.index() == func1_e )
			return std::get<func1_t>(func) != nullptr;

		else if( func.index() == future_func0_e )
			return std::get<future_func0_t>(func) != nullptr;

		else if( func.index() == future_func1_e )
			return std::get<future_func1_t>(func) != nullptr;

		else if( func.index() == await_func0_e )
			return std::get<await_func0_t>(func) != nullptr;

		return std::get<await_func1_t>(func) != nullptr;
	}();
	if( not res )
		throw std::runtime_error("libgs::modules::reg_init: Invalid function object.");
	init_map()[level].emplace_back(std::move(func));
}

string_vector modules::do_init(const string_vector &args)
{
	if( is_run() )
	{
		throw std::runtime_error (
			"libgs::modules::do_init: The executor is already running."
		);
	}
	auto _args = args;
	for(auto &func_vector : std::views::values(init_map()))
	{
		std::vector<func0_t> func0_vector;
		std::vector<func1_t> func1_vector;
		std::vector<std::future<void>> future_vector;

		for(auto &var : func_vector)
		{
			if( var.index() == func0_e )
				func0_vector.emplace_back(std::get<func0_t>(std::move(var)));

			else if( var.index() == func1_e )
				func1_vector.emplace_back(std::get<func1_t>(std::move(var)));

			else if( var.index() == future_func0_e )
				future_vector.emplace_back(std::get<future_func0_t>(var)());

			else if( var.index() == future_func1_e )
				future_vector.emplace_back(std::get<future_func1_t>(var)(_args));

			else if( var.index() == await_func0_e )
			{
				future_vector.emplace_back (
					dispatch(std::get<await_func0_t>(var)(), use_future)
				);
			}
			else if( var.index() == await_func1_e )
			{
				future_vector.emplace_back (
					dispatch(std::get<await_func1_t>(var)(_args), use_future)
				);
			}
		}
		for(auto &func0 : func0_vector)
			func0();
		for(auto &func1 : func1_vector)
			func1(_args);

		std::thread thread;
		post([&]() mutable
		{
			thread = std::thread ([&]
			{
				for(auto &futrue : future_vector)
					futrue.wait();
				exit();
			});
		});
		exec();
		try { thread.join(); } catch(...){}
	}
	init_map().clear();
	return _args;
}

string_vector modules::do_init(int argc, const char *argv[])
{
	return do_init(string_vector(argv, argv + argc));
}

} //namespace libgs