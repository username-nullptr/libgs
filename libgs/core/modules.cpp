#include "modules.h"
#include <map>

namespace libgs
{

using level_t            = modules::level_t;
using init_func_t        = modules::init_func_t;
using future_init_func_t = modules::future_init_func_t;
using await_init_func_t  = modules::await_init_func_t;
using func_obj_t         = modules::func_obj_t;

using func_list_t = std::list<func_obj_t>;
using func_map_t  = std::map<level_t,func_list_t>;

static inline func_map_t &init_map()
{
	static func_map_t map;
	return map;
}

awaitable<void> modules::co_run_init()
{
	return co_spawn([]() -> awaitable<void>
	{
		auto map = std::move(init_map());
		for(auto &[level, func_list] : map)
		{
			std::list<std::future<void>> future_list;
			for(auto &var : func_list)
			{
				if( var.index() == 0 )
					std::get<init_func_t>(var)();
				else if( var.index() == 1 )
					future_list.emplace_back(std::get<future_init_func_t>(var)());
				else
					co_await std::get<await_init_func_t>(var)();
			}
			for(auto &futrue : future_list)
				co_await co_wait(futrue);
		}
		co_return ;
	});
}

void modules::run_init()
{
	auto map = std::move(init_map());
	for(auto &[level, func_list] : map)
	{
		std::list<std::future<void>> future_list;
		for(auto &var : func_list)
		{
			if( var.index() == 0 )
				std::get<init_func_t>(var)();
			else if( var.index() == 1 )
				future_list.emplace_back(std::get<future_init_func_t>(var)());
		}
		for(auto &futrue : future_list)
			futrue.wait();
	}
}

void modules::reg_init_p(func_obj_t func, level_t level)
{
	assert([&func]() -> bool
	{
		if( func.index() == 0 )
			return std::get<0>(func) != nullptr;
		else if( func.index() == 1 )
			return std::get<1>(func) != nullptr;
		else
			return std::get<2>(func) != nullptr;
		return false;
	}());
	init_map()[level].emplace_back(std::move(func));
}

} //namespace libgs