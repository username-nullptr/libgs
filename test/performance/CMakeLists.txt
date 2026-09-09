add_libgs_performance_test(lock-free-queue lock_free_queue.cpp gs.core)

if (LIBGS_BUILD_HTTP)
	add_libgs_performance_test(http http.cpp gs.http)
endif ()

if (LIBGS_BUILD_UTILITIES)
	add_libgs_performance_test(signal-slot signal_slot.cpp gs.utils)
	add_libgs_performance_test(sbus sbus.cpp gs.utils)
endif ()
