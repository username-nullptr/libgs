// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_TRANSPORT_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_TRANSPORT_IPP

namespace libgs::websocket
{

template <core_concepts::exec Exec>
basic_stream<Exec>::impl::impl(executor_t exec, config_t config)
	: m_exec(std::move(exec)), m_config(config)
{

}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::adopt
(connection_ptr connection, adopt_options_t options, error_code &error) noexcept
{
	if( m_state != connection_state::idle )
	{
		error = make_error_code(errc::already_open);
		return ;
	}
	if( not connection )
	{
		error = make_error_code(std::errc::invalid_argument);
		return ;
	}
	if( m_config.read_buffer_size == 0 )
	{
		error = make_error_code(std::errc::invalid_argument);
		return ;
	}
	if( options.stream_role != role::client and options.stream_role != role::server )
	{
		error = make_error_code(std::errc::invalid_argument);
		return ;
	}
	if( not options.negotiated_extensions.empty() )
	{
		error = make_error_code(errc::unsupported_extension);
		return ;
	}
	if( not connection->is_open() )
	{
		error = make_error_code(std::errc::not_connected);
		return ;
	}
	try {
		auto connection_exec = connection->get_executor();
		if constexpr( requires { connection_exec != m_exec; } )
		{
			if( connection_exec != m_exec )
			{
				error = make_error_code(std::errc::invalid_argument);
				return ;
			}
		}
		m_role = options.stream_role;
		m_parser = std::make_unique<frame_parser>(frame_codec_config {
			.local_role = m_role, .max_frame_size = m_config.max_frame_size
		});
		m_read_buffer = std::make_shared
			<std::vector<std::byte>>(m_config.read_buffer_size);

		m_read_size = 0;
		m_read_offset = 0;
		m_pending_offset = 0;

		m_read_error.clear();
		m_read_active = false;

		m_message_type.reset();
		m_message_body.clear();
		m_control_body.clear();
		m_control_event.reset();

		m_pending_data = std::move(options.pending_data);
		m_subprotocol = std::move(options.negotiated_subprotocol);
		m_extensions = std::move(options.negotiated_extensions);
		m_connection = std::move(connection);

		m_state = connection_state::open;
		error.clear();
	}
	catch(const std::bad_alloc&) {
		error = make_error_code(std::errc::not_enough_memory);
	}
	catch(...) {
		error = make_error_code(std::errc::io_error);
	}
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_TRANSPORT_IPP
