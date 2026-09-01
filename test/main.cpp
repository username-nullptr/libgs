#include <libgs/http/server.h>
#include <libgs/http/client.h>
#include <spdlog/spdlog.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{

template <typename Buffer, typename Source>
concept buffer_data_copyable = requires(Source &&source) {
	libgs::copy_buffer_data<Buffer>(std::forward<Source>(source));
};

static_assert(libgs::is_array_buffer_v<std::array<std::uint32_t,4>>);
static_assert(libgs::is_vector_buffer_v<std::vector<std::uint32_t>>);
static_assert(libgs::is_string_buffer_v<std::string>);

static_assert(not libgs::is_array_buffer_v<std::array<std::string,4>>);
static_assert(not libgs::is_array_buffer_v<std::array<const std::uint32_t,4>>);
static_assert(not libgs::is_vector_buffer_v<std::vector<std::string>>);
static_assert(not libgs::is_vector_buffer_v<std::vector<bool>>);
static_assert(not libgs::is_buffer_v<std::vector<std::string>>);

static_assert(buffer_data_copyable<
	std::vector<std::uint32_t>, std::vector<std::byte>
>);
static_assert(not buffer_data_copyable<
	std::vector<std::string>, std::vector<std::byte>
>);
static_assert(not buffer_data_copyable<
	std::vector<std::byte>, std::vector<std::string>
>);

} //namespace

int main()
{
	using namespace libgs::operators;



	return libgs::exec();
}
