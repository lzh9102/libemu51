enable_testing()

find_library(CMOCKA_LIB NAMES cmocka)

if(CMOCKA_LIB STREQUAL "CMOCKA_LIB-NOTFOUND")
	message(WARNING "cmocka not found; unit tests will not be built!")
else()
	include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)
	add_executable(test_api test_api.c)
	add_test(test_api test_api)
	target_link_libraries(test_api emu51 ${CMOCKA_LIB})

	add_executable(test_jumps test_jumps.c)
	add_test(test_jumps test_jumps)
	target_link_libraries(test_jumps emu51 cmocka)

	add_executable(test_helpers test_helpers.c)
	add_test(test_helpers test_helpers)
	target_link_libraries(test_helpers emu51 cmocka)

	add_executable(test_mem test_mem.c)
	add_test(test_mem test_mem)
	target_link_libraries(test_mem emu51 cmocka)

	if (GCOV_ENABLED)
		add_custom_target(coverage
			sh ${PROJECT_SOURCE_DIR}/tests/coverage-lcov.sh ${PROJECT_BINARY_DIR}
			COMMENT "Generate coverage statistics")
	endif()
endif()
