# SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(LIBGS_HEAVY_COMPILE_JOBS_DEFAULT 8)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(LIBGS_HEAVY_COMPILE_JOBS_DEFAULT 6)
else ()
	set(LIBGS_HEAVY_COMPILE_JOBS_DEFAULT 0)
endif ()

set(LIBGS_HEAVY_COMPILE_JOBS ${LIBGS_HEAVY_COMPILE_JOBS_DEFAULT} CACHE STRING
	"Maximum concurrent memory-heavy HTTP/WebSocket test and example compilations; 0 disables the limit."
)
if (NOT LIBGS_HEAVY_COMPILE_JOBS MATCHES "^[0-9]+$")
	message(FATAL_ERROR
		"${PRO_NAME}: LIBGS_HEAVY_COMPILE_JOBS must be a non-negative integer."
	)
endif ()

option(LIBGS_LOW_MEMORY_DEBUG_INFO
	"-- ${PRO_NAME}: Use reduced GCC debug information to lower compiler memory use." OFF
)
if (LIBGS_LOW_MEMORY_DEBUG_INFO)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(
			$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Debug>>:-g1>
		)
		message(STATUS
			"${PRO_NAME}: Use reduced GCC debug information for lower compile memory."
		)
	else ()
		message(WARNING
			"${PRO_NAME}: LIBGS_LOW_MEMORY_DEBUG_INFO currently supports GCC only."
		)
	endif ()
endif ()

if (LIBGS_HEAVY_COMPILE_JOBS GREATER 0)
	message(STATUS
		"${PRO_NAME}: Limit concurrent heavy compilations to ${LIBGS_HEAVY_COMPILE_JOBS}."
	)
	if (CMAKE_GENERATOR MATCHES "Ninja")
		set_property(GLOBAL APPEND PROPERTY JOB_POOLS
			libgs_heavy_compile=${LIBGS_HEAVY_COMPILE_JOBS}
		)
	endif ()
endif ()


# HTTP/WebSocket tests and examples are mostly one-source executables whose
# template instantiation can still exceed 1 GiB per compiler process.
# Keep light compilation fully parallel while bounding only those known-heavy
# sources. Ninja has native compile pools; other generators use independent
# target lanes so a global -j value can remain high.
function(libgs_limit_heavy_compile target)
	if (LIBGS_HEAVY_COMPILE_JOBS EQUAL 0)
		return()
	endif ()

	if (CMAKE_GENERATOR MATCHES "Ninja")
		set_property(TARGET ${target} PROPERTY JOB_POOL_COMPILE libgs_heavy_compile)
		return()
	endif ()

	get_property(libgs_heavy_index GLOBAL PROPERTY LIBGS_HEAVY_COMPILE_INDEX)

	if (NOT libgs_heavy_index)
		set(libgs_heavy_index 0)
	endif ()

	math(EXPR libgs_heavy_lane
		"${libgs_heavy_index} % ${LIBGS_HEAVY_COMPILE_JOBS}"
	)
	get_property(libgs_heavy_previous GLOBAL PROPERTY
		LIBGS_HEAVY_COMPILE_LANE_${libgs_heavy_lane}
	)
	if (libgs_heavy_previous)
		add_dependencies(${target} ${libgs_heavy_previous})
	endif ()

	set_property(GLOBAL PROPERTY
		LIBGS_HEAVY_COMPILE_LANE_${libgs_heavy_lane} ${target}
	)
	math(EXPR libgs_heavy_index "${libgs_heavy_index} + 1")
	set_property(GLOBAL PROPERTY LIBGS_HEAVY_COMPILE_INDEX ${libgs_heavy_index})
endfunction()
