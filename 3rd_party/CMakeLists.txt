# SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

option(LIBGS_ADD_LIBRARY_VERSION
	"-- ${PRO_NAME}: Add version information to shared library names." ON
)
set(LIBGS_ASIO_PROVIDER "" CACHE STRING
	"-- ${PRO_NAME}: Asio provider (BUNDLED, EXTERNAL, or BOOST); empty uses BUNDLED."
)
set_property(CACHE LIBGS_ASIO_PROVIDER PROPERTY STRINGS BUNDLED EXTERNAL BOOST)

string(STRIP "${LIBGS_ASIO_PROVIDER}" LIBGS_ASIO_PROVIDER)
string(TOUPPER "${LIBGS_ASIO_PROVIDER}" LIBGS_ASIO_PROVIDER)

if (LIBGS_ASIO_PROVIDER STREQUAL "")
	set(LIBGS_ASIO_PROVIDER BUNDLED)

elseif (NOT LIBGS_ASIO_PROVIDER MATCHES "^(BUNDLED|EXTERNAL|BOOST)$")
	message(FATAL_ERROR
		"${PRO_NAME}: Invalid LIBGS_ASIO_PROVIDER='${LIBGS_ASIO_PROVIDER}'. "
		"Expected BUNDLED, EXTERNAL, BOOST, or an empty value."
	)
endif ()

set(LIBGS_BOOST_INSTALL_PREFIX "" CACHE PATH
	"Install prefix of an external Boost package."
)
set(LIBGS_ASIO_INSTALL_PREFIX "" CACHE PATH
	"Install prefix of an external Asio package."
)
set(LIBGS_USING_BOOST_ASIO OFF)

if (LIBGS_ASIO_PROVIDER STREQUAL "BOOST")
	set(LIBGS_USING_BOOST_ASIO ON)
endif ()

option(LIBGS_USE_BUNDLED_SPDLOG
	"-- ${PRO_NAME}: Use bundled spdlog." ON
)
set(LIBGS_SPDLOG_INSTALL_PREFIX "" CACHE PATH
	"Install prefix of an external spdlog package."
)
add_library(libgs.asio INTERFACE)
add_library(LibGS::asio ALIAS libgs.asio)

set_target_properties(libgs.asio PROPERTIES EXPORT_NAME asio)
set(LIBGS_ASIO_PACKAGE_TARGET asio::asio)

if (LIBGS_ASIO_PROVIDER STREQUAL "BOOST")
	message(STATUS "${PRO_NAME}: Use external Boost.Asio.")

	if (LIBGS_BOOST_INSTALL_PREFIX)
		list(PREPEND CMAKE_PREFIX_PATH "${LIBGS_BOOST_INSTALL_PREFIX}")
		set(Boost_ROOT "${LIBGS_BOOST_INSTALL_PREFIX}")
	endif ()

	find_package(Boost CONFIG REQUIRED)
	if (TARGET Boost::headers)
		set(LIBGS_ASIO_PACKAGE_TARGET Boost::headers)
	elseif (TARGET Boost::boost)
		set(LIBGS_ASIO_PACKAGE_TARGET Boost::boost)
	else ()
		message(FATAL_ERROR
			"${PRO_NAME}: The external Boost package does not provide a headers target."
		)
	endif ()

	target_link_libraries(libgs.asio INTERFACE ${LIBGS_ASIO_PACKAGE_TARGET})
	target_compile_definitions(libgs.asio INTERFACE LIBGS_USING_BOOST_ASIO=1)

elseif (LIBGS_ASIO_PROVIDER STREQUAL "BUNDLED")
	message(STATUS "${PRO_NAME}: Use bundled standalone Asio.")

	target_include_directories(libgs.asio SYSTEM INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/asio>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	)
	target_compile_definitions(libgs.asio INTERFACE ASIO_STANDALONE=1)

	add_subdirectory(asio)

elseif (LIBGS_ASIO_PROVIDER STREQUAL "EXTERNAL")
	message(STATUS "${PRO_NAME}: Use external standalone Asio.")
	find_package(asio CONFIG QUIET HINTS "${LIBGS_ASIO_INSTALL_PREFIX}")

	if (NOT TARGET asio::asio AND NOT TARGET Asio::asio)
		find_package(Asio CONFIG QUIET HINTS "${LIBGS_ASIO_INSTALL_PREFIX}")
	endif ()

	if (TARGET asio::asio)
		set(LIBGS_ASIO_PACKAGE_TARGET asio::asio)
	elseif (TARGET Asio::asio)
		set(LIBGS_ASIO_PACKAGE_TARGET Asio::asio)
	else ()
		find_path(LIBGS_ASIO_INCLUDE_DIR
			NAMES asio.hpp
			HINTS "${LIBGS_ASIO_INSTALL_PREFIX}"
			PATH_SUFFIXES include
		)
		if (NOT LIBGS_ASIO_INCLUDE_DIR)
			message(FATAL_ERROR
				"${PRO_NAME}: External standalone Asio was requested, but asio.hpp was not found. "
				"Set LIBGS_ASIO_INSTALL_PREFIX to its install prefix."
			)
		endif ()

		add_library(asio::asio INTERFACE IMPORTED GLOBAL)
		set_target_properties(asio::asio PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${LIBGS_ASIO_INCLUDE_DIR}"
		)
		set(LIBGS_ASIO_PACKAGE_TARGET asio::asio)
		mark_as_advanced(LIBGS_ASIO_INCLUDE_DIR)
	endif ()

	target_link_libraries(libgs.asio INTERFACE ${LIBGS_ASIO_PACKAGE_TARGET})
	target_compile_definitions(libgs.asio INTERFACE ASIO_STANDALONE=1)
endif ()

add_library(libgs.spdlog INTERFACE)
add_library(LibGS::spdlog ALIAS libgs.spdlog)

set_target_properties(libgs.spdlog PROPERTIES EXPORT_NAME spdlog)
set(LIBGS_SPDLOG_PACKAGE_TARGET spdlog::spdlog_header_only)

if (LIBGS_USE_BUNDLED_SPDLOG)
	message(STATUS "${PRO_NAME}: Use bundled spdlog.")

	target_include_directories(libgs.spdlog SYSTEM INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/spdlog>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	)
	add_subdirectory(spdlog)
	set(LIBGS_SPDLOG_PROVIDER bundled)

else ()
	message(STATUS "${PRO_NAME}: Use external spdlog.")
	find_package(spdlog CONFIG QUIET HINTS "${LIBGS_SPDLOG_INSTALL_PREFIX}")

	if (TARGET spdlog::spdlog_header_only)
		set(LIBGS_SPDLOG_PACKAGE_TARGET spdlog::spdlog_header_only)
	elseif (TARGET spdlog::spdlog)
		set(LIBGS_SPDLOG_PACKAGE_TARGET spdlog::spdlog)
	else ()
		find_path(LIBGS_SPDLOG_INCLUDE_DIR
			NAMES spdlog/spdlog.h
			HINTS "${LIBGS_SPDLOG_INSTALL_PREFIX}"
			PATH_SUFFIXES include
		)
		if (NOT LIBGS_SPDLOG_INCLUDE_DIR)
			message(FATAL_ERROR
				"${PRO_NAME}: External spdlog was requested, but spdlog/spdlog.h was not found. "
				"Set LIBGS_SPDLOG_INSTALL_PREFIX to its install prefix."
			)
		endif ()

		add_library(spdlog::spdlog_header_only INTERFACE IMPORTED GLOBAL)
		set_target_properties(spdlog::spdlog_header_only PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${LIBGS_SPDLOG_INCLUDE_DIR}"
		)
		set(LIBGS_SPDLOG_PACKAGE_TARGET spdlog::spdlog_header_only)
		mark_as_advanced(LIBGS_SPDLOG_INCLUDE_DIR)
	endif ()

	target_link_libraries(libgs.spdlog INTERFACE ${LIBGS_SPDLOG_PACKAGE_TARGET})
	set(LIBGS_SPDLOG_PROVIDER external)
endif ()

install(TARGETS libgs.asio EXPORT LibGSTargets)
if (LIBGS_BUILD_UTILITIES)
	install(TARGETS libgs.spdlog EXPORT LibGSTargets)
endif ()

# These values are needed while generating the installed package config in the
# parent directory.
set(LIBGS_ASIO_PROVIDER "${LIBGS_ASIO_PROVIDER}" PARENT_SCOPE)
set(LIBGS_ASIO_PACKAGE_TARGET "${LIBGS_ASIO_PACKAGE_TARGET}" PARENT_SCOPE)

set(LIBGS_SPDLOG_PROVIDER "${LIBGS_SPDLOG_PROVIDER}" PARENT_SCOPE)
set(LIBGS_SPDLOG_PACKAGE_TARGET "${LIBGS_SPDLOG_PACKAGE_TARGET}" PARENT_SCOPE)
