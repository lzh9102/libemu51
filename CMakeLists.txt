cmake_minimum_required(VERSION 2.8)
project(emu51)
enable_testing()

# show all warnings when using gcc
if (CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(doc)
