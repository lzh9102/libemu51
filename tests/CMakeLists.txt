enable_testing()

set(BUILD_TESTS OFF CACHE BOOL "Build unit tests")
if(BUILD_TESTS)
	include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)
	add_executable(tests tests.c)
	add_test(tests tests)
	target_link_libraries(tests emu51 cmocka)

	add_executable(test_instr test_instr.c)
	add_test(test_instr test_instr)
	target_link_libraries(test_instr emu51 cmocka)
endif(BUILD_TESTS)
