// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_UTILS_UTILS_SBUS_DETAIL_CACHE_H
#define LIBGS_UTILS_UTILS_SBUS_DETAIL_CACHE_H

#include <libgs/utils/process.h>
#include <libgs/coro/utils.h>
#include <atomic>

namespace libgs::utils
{

template <concepts::streamer_type_p Tag>
struct arg_converter<std::vector<std::byte>,Tag>
{
	static constexpr bool valid = true;
	using payload_t = std::vector<std::byte>;

	[[nodiscard]] static auto convert(const payload_t &value) requires valid
	{
		if constexpr( concepts::optional_p<Tag> )
		{
			using value_t = std::remove_cvref_t<Tag>::value_t;
			using target_t = optional<value_t>;

			target_t opt;
			if( not value.empty() )
			{
				if constexpr( std::is_same_v<value_t,std::vector<std::byte>> )
					opt = value;
				else
					opt = *streamer<value_t>::decode(value);
			}
			return opt;
		}
		else
		{
			using target_t = std::remove_cvref_t<Tag>;
			if( value.empty() )
				return target_t();

			if constexpr( std::is_same_v<target_t,std::vector<std::byte>> )
				return value;
			else
				return *streamer<target_t>::decode(value);
		}
	}
};

namespace sbus
{

template <concepts::subscriber Subscriber>
class LIBGS_UTILS_TAPI cache<Subscriber>::impl : public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <typename...Args>
	using signal_ptr = std::shared_ptr<signal_t<Args...>>;

	struct cache_t
	{
		LIBGS_META_FIELDS (
			( uint64_t , time, 0 ),
			( payload_t, data    )
		);
	};
	struct cache_event
	{
		LIBGS_UTILS_SBUS_TYPE_IMPL (
			" ___35947__LIBGS_UTILS_SBUS__PRIVATE__CACHE_CACHE_EVENT__27136___ "
		)
		LIBGS_META_FIELDS (
			( std::string, topic    ),
			( uint64_t   , pid  , 0 ),
			( uint64_t   , time , 0 ),
			( payload_t  , data     )
		);
	};

public:
	template <typename Exec0>
	explicit impl(Exec0 &&exec) :
		m_subscriber(std::forward<Exec0>(exec))
	{
		m_subscriber.subscribe (
		[this](std::string_view topic, const void *data, size_t size) -> awaitable<void>
		{
			std::span view {
				static_cast<const std::byte*>(data), size
			};
			payload_t payload = { view.begin(), view.end() };
			auto time = std::numeric_limits<uint64_t>::max();
			std::string _topic(topic);

			if( _topic == cache_event::libgs_sbus_topic_v )
			{
				auto cache = *streamer<cache_event>::decode(payload);
				if( auto pid = process::self_pid(); not pid or cache.pid == *pid )
					co_return ;

				_topic = std::move(cache.topic);
				payload = std::move(cache.data);
				time = cache.time;
			}
			std::unique_lock locker(m_caches_mutex);
			auto &curr = m_caches[_topic];

			if( time < curr.time )
				co_return ;

			curr.time = time;
			if( curr.data.size() == size )
			{
				if( memcmp(curr.data.data(), data, size) == 0 )
					co_return ;
			}
			auto _prev = std::move(curr.data);
			auto _curr = curr.data = std::move(payload);
			locker.unlock();

			signal_ptr<payload_t,payload_t> signal {};
			m_signals_mutex.lock();
			{
				auto &obj = m_signals[_topic];
				if( not obj )
					obj = std::make_shared<signal_t<payload_t,payload_t>>();
				signal = obj;
			}
			m_signals_mutex.unlock();

			co_await signal->emit(_curr, _prev);
			co_await m_signal.emit(_topic, _curr, _prev);
			co_return ;
		});
	}
	~impl() {
		m_subscriber.cancel();
	}

public:
	template <typename T>
	static void decode_payload(T &target, payload_t payload) noexcept
	{
		using type = std::remove_cvref_t<T>;
		if constexpr( std::is_same_v<type,payload_t> )
			target = std::move(payload);

		else if constexpr( libgs::concepts::optional<type> )
		{
			using value_t = type::value_t;
			if( payload.empty() )
				return ;

			if constexpr( std::is_same_v<value_t,payload_t> )
				target = std::move(payload);

			else if constexpr( libgs::concepts::streamer_type<value_t> )
				target = *streamer<value_t>::decode(payload);

			else if( payload.size() >= sizeof(value_t) )
				target = *reinterpret_cast<const value_t*>(payload.data());
		}
		else if constexpr( libgs::concepts::streamer_type<type> )
		{
			if( not payload.empty() )
				target = *streamer<type>::decode(payload);
		}
		else
		{
			if( payload.size() >= sizeof(type) )
				target = *reinterpret_cast<const type*>(payload.data());
		}
	}

	template <typename T>
	[[nodiscard]] static changed_result<T> decode_changed(payload_t curr, payload_t prev) noexcept
	{
		changed_result<T> result {};
		decode_payload(result.current, std::move(curr));
		decode_payload(result.previous, std::move(prev));
		return result;
	}

public:
	void set(std::string_view topic, const void *data, size_t size)
	{
		std::unique_lock locker(m_caches_mutex);
		auto &curr = m_caches[std::string(topic)];

		if( curr.data.size() == size )
		{
			if( memcmp(curr.data.data(), data, size) == 0 )
				return ;
		}
		auto _prev = std::move(curr.data);
		std::span view {
			static_cast<const std::byte*>(data), size
		};
		auto _curr = curr.data = { view.begin(), view.end() };
		locker.unlock();

		dispatch(m_subscriber.get_executor(), [this, delivery_topic = std::string(topic),
			previous_payload = std::move(_prev), current_payload = _curr]() -> awaitable<void>
		{
			signal_ptr<payload_t,payload_t> signal {};
			m_signals_mutex.lock();
			{
				auto &obj = m_signals[delivery_topic];
				if( not obj )
					obj = std::make_shared<signal_t<payload_t,payload_t>>();
				signal = obj;
			}
			m_signals_mutex.unlock();

			co_await signal->emit(current_payload, previous_payload);
			co_await m_signal.emit(delivery_topic, current_payload, previous_payload);
			co_return ;
		});
		auto pid = process::self_pid();
		if( not pid )
			return ;

		cache_event event {
			.topic = std::string(topic),
			.pid   = *pid              ,
			.data  = std::move(_curr)
		};
		event.time = std::chrono::system_clock::now().time_since_epoch().count();
		publish<interface_t>(std::move(event));
	}

	template <libgs::concepts::any_string_p Str>
	void set(std::string_view topic, Str &&str)
	{
		auto view = strtls::to_view(std::forward<Str>(str));
		set(topic, view.data(), view.size());
	}

	template <concepts::unregistered_type_p T>
	void set(std::string_view topic, const T &data)
	{
		using type = std::remove_cvref_t<T>;
		if constexpr( libgs::concepts::streamer_type<type> )
		{
			auto payload = streamer<type>::encode(data);
			set(topic, payload.data(), payload.size());
		}
		else if constexpr( std::is_same_v<type, const_buffer> or
			std::is_same_v<type, asio::const_buffer> )
			set(topic, data.data(), data.size());
		else
			set(topic, &data, sizeof(data));
	}

	void set(concepts::topic_type auto &&data)
	{
		using T = decltype(data);
		using type = std::remove_cvref_t<T>;

		if constexpr( libgs::concepts::streamer_type<type> )
		{
			auto payload = streamer<type>::encode(data);
			set(type::libgs_sbus_topic_v, payload.data(), payload.size());
		}
		else
			set(type::libgs_sbus_topic_v, &data, sizeof(data));
	}

public:
	signal_t<payload_t,payload_t> &changed(std::string_view topic) noexcept
	{
		std::unique_lock locker(m_signals_mutex); LIBGS_UNUSED(locker);
		auto &signal = m_signals[std::string(topic)];
		if( not signal )
			signal = std::make_shared<signal_t<payload_t,payload_t>>();
		return *signal;
	}

public:
	template <typename T = payload_t>
	[[nodiscard]] sys_expected<changed_result<T>> wait_changed(std::string_view topic) noexcept
	{
		auto observer = std::make_shared<int>();
		asio::io_context ioc;
		exec_detach(ioc);

		std::condition_variable cond_var;
		changed_result<T> result {};

		changed(topic).connect(observer, ioc,
		[&, observer](payload_t curr, payload_t prev) mutable noexcept
		{
			if( observer.use_count() == 1 )
				return ;

			result = decode_changed<T>(std::move(curr), std::move(prev));
			cond_var.notify_one();
		});
		std::mutex mutex;
		std::unique_lock locker(mutex);
		cond_var.wait(locker);

		ioc.stop();
		changed(topic).disconnect(observer);
		return { result };
	}

	template <typename T = payload_t>
	[[nodiscard]] awaitable<sys_expected<changed_result<T>>> co_wait_changed(std::string_view topic,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using worker_t = async_work<std::error_code,changed_result<T>>;
		using work_handler_t = worker_t::handler_t;

		auto exec = m_subscriber.get_executor();
		auto cancel_state = co_await asio::this_coro::cancellation_state;

		auto task = worker_t::handle(exec, [this,
			topic = std::string(topic), exec, cancel_state, cancel_slot
		](work_handler_t &&notifier) mutable noexcept
		{
			auto observer = std::make_shared<int>();
			auto notifier_ptr = std::make_shared<work_handler_t>(std::move(notifier));
			auto completed = std::make_shared<std::atomic_bool>(false);

			auto canceller = [this, topic, exec, observer, completed, notifier_ptr]
			(asio::cancellation_type type) mutable noexcept
			{
				if( type == asio::cancellation_type::none or completed->exchange(true) )
					return ;
				changed(topic).disconnect(observer);

				libgs::dispatch(exec, [
					delivery_topic = std::move(topic), observer,
					delivery_notifier = std::move(notifier_ptr)
				]() mutable noexcept
				{
					LIBGS_UNUSED(delivery_topic);
					std::move(*delivery_notifier) (
						asio::error::make_error_code(asio::error::operation_aborted),
						changed_result<T>()
					);
				});
			};
			if( cancel_slot.is_connected() )
				cancel_slot.assign(canceller);

			if( cancel_state.slot().is_connected() )
				cancel_state.slot().assign(std::move(canceller));

			changed(topic).connect(observer, std::move(exec),
			[this, topic, observer, completed, change_notifier = std::move(notifier_ptr)]
			(payload_t curr, payload_t prev) mutable noexcept
			{
				if( completed->exchange(true) )
					return ;
				changed(topic).disconnect(observer);

				std::move(*change_notifier)(std::error_code(),
					decode_changed<T>(std::move(curr), std::move(prev))
				);
			});
		},
		use_awaitable);

		using namespace std::chrono_literals;
		sys_expected<changed_result<T>> expected {};

		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_subscriber.get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::get<0>(var);
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		co_return expected;
	}

	[[nodiscard]] awaitable<io_expected> co_wait_changed(std::error_code &error, std::string_view topic,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_wait_changed(topic,
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	subscriber_t m_subscriber {};
	std::unordered_map<std::string,cache_t> m_caches {};
	spin_shared_mutex m_caches_mutex {};

	signal_t<std::string_view,payload_t,payload_t> m_signal {};
	std::unordered_map<std::string,signal_ptr<payload_t,payload_t>> m_signals {};
	spin_mutex m_signals_mutex {};
};

template <concepts::subscriber Subscriber>
template <typename Exec0>
cache<Subscriber>::cache(Exec0 &&exec)
	requires libgs::concepts::match_sched<Exec0,executor_t> :
	m_impl(std::make_shared<impl>(std::forward<Exec0>(exec)))
{

}

template <concepts::subscriber Subscriber>
cache<Subscriber>::~cache() = default;

template <concepts::subscriber Subscriber>
cache<Subscriber> &cache<Subscriber>::set(std::string_view topic, const void *data, size_t size)
{
	m_impl->set(topic, data, size);
	return *this;
}

template <concepts::subscriber Subscriber>
template <libgs::concepts::any_string_p...Args>
cache<Subscriber> &cache<Subscriber>::set(std::string_view topic, Args&&...args)
	requires (sizeof...(Args) > 0)
{
	(void) std::initializer_list<int> {
		(m_impl->set(topic, std::forward<Args>(args)), 0) ...
	};
	return *this;
}

template <concepts::subscriber Subscriber>
template <concepts::unregistered_type_p...Args>
cache<Subscriber> &cache<Subscriber>::set(std::string_view topic, Args&&...args)
	requires (sizeof...(Args) > 0)
{
	(void) std::initializer_list<int> {
		(m_impl->set(topic, std::forward<Args>(args)), 0) ...
	};
	return *this;
}

template <concepts::subscriber Subscriber>
template <concepts::topic_type...Args>
cache<Subscriber> &cache<Subscriber>::set(Args&&...args)
	requires (sizeof...(Args) > 0)
{
	(void) std::initializer_list<int> {
		(m_impl->set(std::forward<Args>(args)), 0) ...
	};
	return *this;
}

template <concepts::subscriber Subscriber>
template <concepts::topic_type T>
optional<T> cache<Subscriber>::get() const
{
	using type = std::remove_cvref_t<T>;
	auto payload = get(type::libgs_sbus_topic_v);
	if( payload.empty() )
		return {};

	if constexpr( libgs::concepts::streamer_type<type> )
		return *streamer<type>::decode(payload);
	else
		return *reinterpret_cast<const type*>(payload.data());
}

template <concepts::subscriber Subscriber>
template <typename T>
optional<T> cache<Subscriber>::get(std::string_view topic) const
{
	using type = std::remove_cvref_t<T>;
	if constexpr( concepts::topic_type<type> )
	{
		if( topic != type::libgs_sbus_topic_v )
			invalid_argument::loc_throw("Topic does not match.");
	}
	spin_shared_shared_lock locker(m_impl->m_caches_mutex);
	auto payload = m_impl->m_caches[std::string(topic)];

	if( payload.data.empty() )
		return {};

	locker.unlock();
	if constexpr( libgs::concepts::streamer_type<type> )
		return *streamer<type>::decode(payload.data);
	else
		return *reinterpret_cast<const type*>(payload.data());
}

template <concepts::subscriber Subscriber>
auto cache<Subscriber>::get(std::string_view topic) const -> payload_t
{
	spin_shared_shared_lock locker(m_impl->m_caches_mutex); LIBGS_UNUSED(locker);
	return m_impl->m_caches[std::string(topic)].data;
}

template <concepts::subscriber Subscriber>
auto cache<Subscriber>::get() const noexcept -> std::map<std::string,payload_t>
{
	std::map<std::string,payload_t> map;
	m_impl->m_caches_mutex.lock_shared();

	for(auto &[topic, cached_entry] : m_impl->m_caches)
		map.emplace(topic, cached_entry.data);

	m_impl->m_caches_mutex.unlock_shared();
	return map;
}

template <concepts::subscriber Subscriber>
auto cache<Subscriber>::changed(std::string_view topic) noexcept -> signal_t<payload_t,payload_t>&
{
	return m_impl->changed(topic);
}

template <concepts::subscriber Subscriber>
auto cache<Subscriber>::changed() noexcept -> signal_t<std::string_view,payload_t,payload_t>&
{
	return m_impl->m_signal;
}

template <concepts::subscriber Subscriber>
template <concepts::topic_type T>
auto cache<Subscriber>::changed() noexcept -> signal_t<payload_t,payload_t>&
{
	using type = std::remove_cvref_t<T>;
	return changed(type::libgs_sbus_topic_v);
}

template <concepts::subscriber Subscriber>
template <concepts::topic_type T, typename Token>
auto cache<Subscriber>::wait_changed(Token &&token) noexcept
	requires is_token_v<Token,optional<T>>
{
	using type = std::remove_cvref_t<T>;
	return wait_changed<T>(type::libgs_sbus_topic_v, std::forward<Token>(token));
}

template <concepts::subscriber Subscriber>
template <typename Token>
auto cache<Subscriber>::wait_changed(std::string_view topic, Token &&token) noexcept
	requires is_token_v<Token>
{
	return wait_changed<payload_t>(topic, std::forward<Token>(token));
}

template <concepts::subscriber Subscriber>
template <typename T, typename Token>
auto cache<Subscriber>::wait_changed(std::string_view topic, Token &&token) noexcept
	requires is_token_v<Token,T>
{
	using type = std::remove_cvref_t<T>;
	if constexpr( concepts::topic_type<type> )
	{
		if( topic != type::libgs_sbus_topic_v )
			invalid_argument::loc_throw("Topic does not match.");
	}
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->template wait_changed<T>(topic)
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->template wait_changed<T>(topic);

	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

		decltype(auto) nntoken = unbound_token(ntoken);
		using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

		if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				return m_impl->template co_wait_changed<T>(ntoken.ec_, topic,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->template co_wait_changed<T>(topic,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_use_future_v<nntoken_t> )
		{
			auto result_promise = std::make_shared<std::promise<io_expected>>();
			auto future = result_promise->get_future();
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					ntoken, wait_topic = std::string(topic), promise = std::move(result_promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_wait_changed<T> (
						ntoken.ec_, wait_topic, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					wait_topic = std::string(topic), promise = std::move(result_promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_wait_changed<T> (
						wait_topic, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return future;
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(get_executor(), [
				impl = m_impl->shared_from_this(), ntoken, nntoken,
				topic = std::string(topic), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_wait_changed<T> (
					ntoken.ec_, topic, cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
		else
		{
			libgs::dispatch(get_executor(), [
				impl = m_impl->shared_from_this(), nntoken,
				topic = std::string(topic), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_wait_changed<T> (
					topic, cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
	}
	else
	{
		using namespace operators;
		using namespace std::chrono_literals;
		return wait_changed<T>(topic, token | 0ns);
	}
}

template <concepts::subscriber Subscriber>
auto cache<Subscriber>::subscriber() noexcept -> subscriber_t
{
	return m_impl->m_subscriber;
}

template <concepts::subscriber Subscriber>
auto cache<Subscriber>::get_executor() noexcept -> executor_t
{
	return subscriber().get_executor();
}

}} //namespace libgs::utils::sbus


#endif //LIBGS_UTILS_UTILS_SBUS_DETAIL_CACHE_H
