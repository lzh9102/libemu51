enable_testing()

set(BUILD_TESTS OFF CACHE BOOL "Build unit tests")
if(BUILD_TESTS)
include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(tests tests.c)
add_test(tests tests)
target_link_libraries(tests emu51 cmocka)
endif(BUILD_TESTS)
