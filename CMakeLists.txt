cmake_minimum_required(VERSION 2.8)
project(emu51)
enable_testing()

# show all warnings when using gcc
if (CMAKE_COMPILER_IS_GNUCC)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_C_FLAGS "-g -O0 -Wall -W -fprofile-arcs -ftest-coverage")
		set(CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")
		set(CMAKE_MODULE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")
		set(GCOV_ENABLED true)
	endif()
endif()

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(doc)
