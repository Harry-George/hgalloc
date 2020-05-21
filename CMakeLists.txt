cmake_minimum_required(VERSION 3.16)
project(GrowingGlobalPoolAllocator)
set(CMAKE_CXX_STANDARD 20)

# Overwriting existing flags with our owns (so we dont define NDEBUG)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic")

include(ExternalProject)

# Get gtest
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
		RESULT_VARIABLE result
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
	message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
		RESULT_VARIABLE result
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
	message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
		${CMAKE_CURRENT_BINARY_DIR}/googletest-build
		EXCLUDE_FROM_ALL)

include(FetchContent)
FetchContent_Declare(googlebenchmark
#		URL https://github.com/google/benchmark/archive/v1.5.0.zip
		GIT_REPOSITORY https://github.com/google/benchmark
		)

enable_testing()

function(register_test)
	cmake_parse_arguments(
			TEST
			"" # Flags.
			"TEST" # Unary arguments.
			"SOURCES" # Multi-value arguments.
			${ARGN}
	)

	add_executable(${TEST_TEST} ${TEST_SOURCES})

	add_dependencies(${TEST_TEST} gtest_main gmock_main)
	target_link_libraries(${TEST_TEST} gtest_main gmock_main)
	include_directories("${gtest_SOURCE_DIR}/include")
	include_directories("${gmock_SOURCE_DIR}/include")

	add_test(
		NAME   ${TEST_TEST}
		COMMAND ${TEST_TEST}
	)

endfunction()

function(register_perf_test)
	cmake_parse_arguments(
			TEST
			"" # Flags.
			"TEST" # Unary arguments.
			"SOURCES" # Multi-value arguments.
			${ARGN}
	)

	add_executable(${TEST_TEST} ${TEST_SOURCES})

#	add_dependencies(${TEST_TEST} benchmark)

	FetchContent_MakeAvailable(googlebenchmark)
	target_link_libraries(${TEST_TEST} benchmark::benchmark)
	include_directories("${CMAKE_CURRENT_BINARY_DIR}/benchmark-src")

	add_test(
		NAME   ${TEST_TEST}
		COMMAND ${TEST_TEST}
	)

endfunction()

register_test(
		TEST testGrowingGlobalPoolAllocator
		SOURCES test/testGrowingGlobalPoolAllocator.cpp
)

register_test(
		TEST testFourByteScopedPtr
		SOURCES test/testFourByteScopedPtr.cpp
)

register_test(
		TEST testGrowingGlobalPoolAllocatorAssertions
		SOURCES test/testGrowingGlobalPoolAllocatorAssertions.cpp
)

register_perf_test(
		TEST perfGrowingGlobalPoolAllocator
		SOURCES test/perfGrowingGlobalPoolAllocator.cpp
)

