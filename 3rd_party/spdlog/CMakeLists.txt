install(DIRECTORY spdlog DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	# gs.utils exports the compiled spdlog implementation.  Installing the
	# header-only implementation beside it would allow consumers to instantiate
	# a second, ABI-incompatible copy.
	PATTERN "*-inl.h" EXCLUDE
)
