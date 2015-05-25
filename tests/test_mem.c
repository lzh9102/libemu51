/* tests for memory-related instructions */

#include "test_instr_common.h"

/* disable unused parameter warning when using gcc */
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

void test_movc(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;

	/* MOVC A, @A+DPTR */
	uint8_t opcode = 0x93;
	int err;
	SET_DPTR(m, 150);
	m->sfr[SFR_ACC] = 7;
	data->pmem[150] = 1; /* this is not the data to read */
	data->pmem[157] = 2; /* this is the data to read (DPTR+ACC) */
	expect_value(callback_sfr_update, index, SFR_ACC);
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->sfr[SFR_ACC], 2);
	assert_emu51_callbacks(data, CB_SFR_UPDATE);

	/* test boundary condition */
	SET_DPTR(m, PMEM_SIZE-1); /* in bounds */
	m->sfr[SFR_ACC] = 0;
	expect_value(callback_sfr_update, index, SFR_ACC);
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, 0);
	assert_emu51_callbacks(data, CB_SFR_UPDATE);

	SET_DPTR(m, PMEM_SIZE); /* out of bounds */
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, EMU51_PMEM_OUT_OF_RANGE);
	assert_emu51_callbacks(data, 0);

	SET_DPTR(m, 0);

	/* MOVC A, @A+PC */
	opcode = 0x83;
	m->pc = 140;
	m->sfr[SFR_ACC] = 7;
	data->pmem[140] = 1; /* this is not the data to read */
	data->pmem[147] = 2; /* this is the data to read (DPTR+ACC) */
	expect_value(callback_sfr_update, index, SFR_ACC);
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->sfr[SFR_ACC], 2);
	assert_emu51_callbacks(data, CB_SFR_UPDATE);

	/* test boundary condition */
	m->pc = PMEM_SIZE - 1; /* in bounds */
	m->sfr[SFR_ACC] = 0;
	expect_value(callback_sfr_update, index, SFR_ACC);
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, 0);
	assert_emu51_callbacks(data, CB_SFR_UPDATE);

	m->pc = PMEM_SIZE; /* out of bounds */
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, EMU51_PMEM_OUT_OF_RANGE);
	assert_emu51_callbacks(data, 0);

	free_test_data(data);
}

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_movc),
	};
	/* don't use setup and teardown as cmocka doesn't report memory bugs in them
	 */
	return cmocka_run_group_tests(tests, NULL, NULL);
}
