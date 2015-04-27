enable_testing()

find_library(CMOCKA_LIB NAMES cmocka)

if(CMOCKA_LIB STREQUAL "CMOCKA_LIB-NOTFOUND")
	message(WARNING "cmocka not found; unit tests will not be built!")
else()
	include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)
	add_executable(tests tests.c)
	add_test(tests tests)
	target_link_libraries(tests emu51 ${CMOCKA_LIB})

	add_executable(test_instr test_instr.c)
	add_test(test_instr test_instr)
	target_link_libraries(test_instr emu51 cmocka)

	if (GCOV_ENABLED)
		add_custom_target(coverage
			sh ${PROJECT_SOURCE_DIR}/tests/coverage-lcov.sh ${PROJECT_BINARY_DIR}
			COMMENT "Generate coverage statistics")
	endif()
endif()
