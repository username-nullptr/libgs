#if defined(_WIN32)
# define LIBGS_EXAMPLE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
# define LIBGS_EXAMPLE_EXPORT __attribute__((visibility("default")))
#else
# define LIBGS_EXAMPLE_EXPORT
#endif

extern "C" LIBGS_EXAMPLE_EXPORT int libgs_example_twice(int value)
{
	return value * 2;
}
