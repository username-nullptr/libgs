#include "modules.h"
#include <unordered_map>
#include <unordered_set>
#include <iostream>

namespace libgs
{

enum variant_type {
	func0_e, func1_e, func_state
};
using state_t = detail::modules::state;
using func_obj_t = detail::modules::func_obj_t;
using dependency_t = modules::dependency;

class LIBGS_DECL_HIDDEN initializer
{
	LIBGS_DISABLE_COPY_MOVE(initializer)
	initializer() = default;

	struct node_t
	{
		func_obj_t init;
		std::atomic_size_t counter {0};

		using ptr_t = std::shared_ptr<node_t>;
		std::unordered_map<std::string,ptr_t> children;
	};
	using node_ptr = node_t::ptr_t;
	using dsd_t = std::unordered_map<std::string,node_ptr>;

public:
	[[nodiscard]] static initializer &instance() noexcept
	{
		static initializer obj;
		return obj;
	}

public:
	void join(std::string name, dependency_t depy, func_obj_t func)
	{
		if( name.empty() )
		{
			throw runtime_error (
				"libgs::modules::reg_init: Empty module name."
			);
		}
		if( m_names.contains(name) )
		{
			throw runtime_error (
				"libgs::modules::reg_init: Module name '{}' already registered.",
				name
			);
		}
		auto res = [&func]() -> bool
		{
			if( func.index() == func0_e )
				return std::get<detail::modules::func0_t>(func) != nullptr;
			// else if( func.index() == func1_e )
			return std::get<detail::modules::func1_t>(func) != nullptr;
		}();
		if( not res )
		{
			throw runtime_error (
				"libgs::modules::reg_init: Invalid function object."
			);
		}
		dsd_emplace(name, std::move(depy), std::move(func));
		m_names.emplace(std::move(name));
	}

	void operator()(string_vector args, std::function<void()> callback)
	{
		auto cycle = detect_cycle();
		if( not cycle.empty() )
		{
			std::string text;
			for(size_t i=0; i<cycle.size()-1; i++)
				text += cycle[i] + " -> ";
			text += cycle.back();

			throw runtime_error (
				"libgs::modules::reg_init: Circular dependency detected: {}",
				text
			);
		}
		init(std::move(args), std::move(callback));
	}

public:
	[[nodiscard]] std::string sprint_dsd() const noexcept
	{
		std::string buffer;
		if( m_dsd.empty() )
		{
			buffer = "empty dsd";
			return buffer;
		}
		std::unordered_set<node_ptr> fully_expanded;
		std::unordered_set<node_ptr> current_path;
		auto it = m_dsd.begin();

		for(; std::next(it)!=m_dsd.end(); ++it)
		{
			sprint_node(it->second, it->first,
				fully_expanded, current_path, buffer, "", false
			);
		}
		sprint_node(it->second, it->first,
			fully_expanded, current_path, buffer, "", true
		);
		buffer.pop_back();
		return buffer;
	}

private:
	node_ptr dsd_emplace(const std::string &name, dependency_t depy, func_obj_t func)
	{
		auto ptr = make_node(std::move(func));
		if( depy.before.empty() and depy.after.empty() )
		{
			if( auto [dsd, it] = find_node(name); it != dsd->end() )
			{
				it->second->init = std::move(ptr->init);
				return it->second;
			}
		}
		++m_counter;

		for(auto &parent_name : depy.after)
		{
			if( parent_name == name )
			{
				throw runtime_error (
					"libgs::modules::reg_init: In 'after', "
					"'{}' is dependent on itself.",
					name
				);
			}
			if( auto it = std::ranges::find(depy.before, parent_name); it != depy.before.end() )
			{
				throw runtime_error (
					"libgs::modules::reg_init: There is a module with the same name "
					"between [before:'{}'] and [after:'{}'] (Circular dependency).",
					parent_name, *it
				);
			}
			if( auto [parents, parent_it] = find_node(parent_name); parent_it == parents->end() )
			{
				make_node(state_t::not_register)->children.emplace(name, ptr);
				++ptr->counter;
			}
			else
			{
				auto &children = parent_it->second->children;
				if( auto child_it = children.find(name); child_it == children.end() )
				{
					children.emplace(name, ptr);
					++ptr->counter;
				}
				else
				{
					child_it->second->init = std::move(ptr->init);
					ptr = child_it->second;
					--m_counter;
				}
			}
		}
		for(auto &child_name : depy.before)
		{
			if( child_name == name )
			{
				throw runtime_error (
					"libgs::modules::reg_init: In 'before', "
					"'{}' is dependent on itself.",
					name
				);
			}
			if( auto [children, child_it] = find_node(child_name); child_it == children->end() )
			{
				auto child = make_node(state_t::not_register);
				++child->counter;
				ptr->children.emplace(child_name, std::move(child));
			}
			else if( auto it = ptr->children.find(child_name); it == ptr->children.end() )
			{
				ptr->children.emplace(child_name, child_it->second);
				++child_it->second->counter;

				if( it = m_dsd.find(child_name); it != m_dsd.end() )
				{
					m_dsd.erase(it);
					--child_it->second->counter;
				}
			}
		}
		if( ptr->counter == 0 )
			ptr->counter = 1;
		if( depy.after.empty() and ptr->counter == 1 )
			m_dsd.emplace(name, ptr);
		return ptr;
	}

private:
	void init(string_vector args, std::function<void()> callback)
	{
		bool sync = not callback;
		std::thread thread([this, args = std::move(args), callback = std::move(callback)]() mutable
		{
			do_init(m_dsd, args);

			std::mutex mutex;
			std::unique_lock locker(mutex);

			m_condition.wait(locker, [this] {
				return m_counter == 0;
			});
			if( callback )
				callback();
		});
		if( sync )
			thread.join();
		else
			thread.detach();
	}

	void do_init(const dsd_t &nodes, const string_vector &args)
	{
		for(auto &[name, node] : nodes)
		{
			if( --node->counter > 0 )
				continue;

			std::thread([this, name, node, &args]() mutable
			{
				if( node->init.index() == func0_e or node->init.index() == func1_e )
				{
					if( node->init.index() == func0_e )
						std::get<detail::modules::func0_t>(std::move(node->init))();
					else if( node->init.index() == func1_e )
						std::get<detail::modules::func1_t>(std::move(node->init))(args);

					node->init = detail::modules::state::finished;
					if( --m_counter == 0 )
					{
						m_condition.notify_all();
						return ;
					}
				}
				else /* if( node->init.index() == func_state ) */
				{
					if( std::get<state_t>(std::move(node->init)) == state_t::not_register )
					{
						auto text = std::format(
							"libgs::modules::reg_init: Module '{}' is not registered.",
							name
						);
						std::cerr << text << std::endl;
						throw runtime_error(text);
					}
				}
				do_init(std::move(node->children), args);
			})
			.detach();
		}
	}

private:
	[[nodiscard]] std::pair<dsd_t*,dsd_t::iterator> find_node(std::string_view name) noexcept {
		return do_find_node(&m_dsd, name);
	}

	[[nodiscard]] static std::pair<dsd_t*,dsd_t::iterator> do_find_node
	(dsd_t *dsd, std::string_view name) noexcept
	{
		for(auto it=dsd->begin(); it!=dsd->end(); ++it)
		{
			if( (*it).first == name )
				return { dsd, it };

			if( auto [_dsd, _it] = do_find_node(&it->second->children, name);
				_it != _dsd->end() )
				return { _dsd, _it };
		}
		return { dsd, dsd->end() };
	}

	[[nodiscard]] node_ptr make_node(func_obj_t init) noexcept
	{
		auto n = std::make_shared<node_t>();
		n->init = std::move(init);
		return n;
	}

private:
	[[nodiscard]] std::vector<std::string> detect_cycle()
	{
		std::unordered_set<std::string> visited; // Fast record stack
		std::vector<std::string> path; // Record stack (in reverse order)

		do_detect_cycle(m_dsd, visited, path);
		std::ranges::reverse(path);
		return path;
	}

	bool do_detect_cycle(const dsd_t &dsd, std::unordered_set<std::string> &visited, std::vector<std::string> &path)
	{
		for(auto &[child_name, child_node] : dsd)
		{
			// Push stack.
			// If the nodes are duplicated, it indicates the presence of a loop.
			if( auto [it, inserted] = visited.emplace(child_name); not inserted )
			{
				path.emplace_back(child_name);
				break;
			}
			// Deep-first search.
			if( do_detect_cycle(child_node->children, visited, path) )
				return true;

			// Pop stack.
			visited.erase(child_name);
			if( not path.empty() )
			{
				// Push stack.
				path.emplace_back(child_name);

				// Stack is full.
				if( path.front() == child_name )
					return true;
				break;
			}
		}
		return false;
	}

public:
	static void sprint_node(const node_ptr &node,
							const std::string &name,
							std::unordered_set<node_ptr> &fully_expanded,
							std::unordered_set<node_ptr> &current_path,
							std::string &buffer,
							const std::string &prefix,
							bool is_last)
	{
		if( current_path.find(node) != current_path.end() )
		{
			buffer += std::format("{}{}{} (Circular)\n",
				prefix, is_last ? "└─" : "├─", name
			);
			return ;
		}
		bool is_already_expanded = fully_expanded.find(node) != fully_expanded.end();
		std::string reg_state;

		if( node->init.index() == func_state )
		{
			if( std::get<state_t>(node->init) == state_t::not_register )
				reg_state = "[NoReg]";
		}
		buffer += std::format("{}{}{}{}{}\n",
			prefix, is_last ? "└─" : "├─", name, reg_state,
			is_already_expanded ? " (Unfolded)" : ""
		);
		if( is_already_expanded )
			return ;

		// Detect whether there is a circular reference.
		current_path.emplace(node);

		auto child_prefix = prefix + (is_last ? "  " : "│ ");
		auto &children = node->children;

		if( not children.empty() )
		{
			auto it = children.begin();
			for(; std::next(it)!=children.end(); ++it)
			{
				sprint_node(it->second, it->first,
					fully_expanded, current_path, buffer, child_prefix, false
				);
			}
			sprint_node(it->second, it->first,
				fully_expanded, current_path, buffer, child_prefix, true
			);
		}
		// Pop stack.
		current_path.erase(node);
		fully_expanded.emplace(node);
	}

private:
	std::unordered_set<std::string> m_names {};
	std::condition_variable m_condition {};
	std::atomic_size_t m_counter {0};
	dsd_t m_dsd {};
};

namespace detail
{

void modules::reg_init(std::string name, dependency_t depy, func_obj_t func)
{
	initializer::instance().join(std::move(name), std::move(depy), std::move(func));
}

void modules::do_init(const string_vector &args, std::function<void()> callback)
{
	initializer::instance()(args, std::move(callback));
}

} //namespace detail

std::string modules::sprint() noexcept
{
	return initializer::instance().sprint_dsd();
}

} //namespace libgs