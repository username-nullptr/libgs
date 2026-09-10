set(LIBGS_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})


# Existing tests form the functional suite. Keeping their target and CTest
# names stable avoids breaking commands that select a test by name.
function(add_libgs_test name source library)
	set(test_target libgs.test.${name})
	add_executable(${test_target} ${source})

	target_link_libraries(${test_target} PRIVATE ${library})
	if (LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN)
		target_link_libraries(${test_target} PRIVATE libgs.test.sanitizer)
	endif ()
	target_include_directories(${test_target} PRIVATE ${LIBGS_TEST_SOURCE_DIR})

	set_target_properties(${test_target} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
	)
	add_test(NAME libgs.${name} COMMAND $<TARGET_FILE:${test_target}> ${ARGN})
	set_tests_properties(libgs.${name} PROPERTIES
		LABELS functional
		FIXTURES_SETUP libgs.functional
		TIMEOUT 60
	)
	if (LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN)
		set_property(TEST libgs.${name} APPEND PROPERTY LABELS sanitizer)
		set_property(TEST libgs.${name} PROPERTY RUN_SERIAL TRUE)
		set_property(TEST libgs.${name} APPEND PROPERTY FAIL_REGULAR_EXPRESSION
			"AddressSanitizer" "LeakSanitizer" "ThreadSanitizer" "runtime error:"
		)
		if (LIBGS_ENABLE_TEST_SANITIZERS)
			set_property(TEST libgs.${name} APPEND PROPERTY ENVIRONMENT
				"ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:abort_on_error=1"
				"UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1"
			)
		else ()
			set_property(TEST libgs.${name} APPEND PROPERTY ENVIRONMENT
				"TSAN_OPTIONS=halt_on_error=1"
			)
		endif ()
	endif ()
endfunction()


function(add_libgs_performance_test name source library)
	set(test_target libgs.test.performance.${name})
	add_executable(${test_target} ${source})

	target_link_libraries(${test_target} PRIVATE ${library})
	target_include_directories(${test_target} PRIVATE ${LIBGS_TEST_SOURCE_DIR})

	set_target_properties(${test_target} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
	)
	add_test(NAME libgs.performance.${name} COMMAND $<TARGET_FILE:${test_target}>)
	set_tests_properties(libgs.performance.${name} PROPERTIES
		LABELS performance
		FIXTURES_REQUIRED libgs.functional
		RUN_SERIAL TRUE
		TIMEOUT 60
	)
endfunction()


add_subdirectory(functional)
if (LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN)
	message(STATUS "${PRO_NAME}: Performance tests disabled in sanitizer builds.")
else ()
	add_subdirectory(performance)
endif ()
