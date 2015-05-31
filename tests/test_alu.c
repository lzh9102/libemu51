/* tests for arithmetic and logical instructions */

#include "test_instr_common.h"

struct arith_testcase {
	uint8_t reg;      /* original value of the target register */
	uint8_t operand;  /* value of the operand */
	uint8_t carry_in; /* carry in (0 or 1) */
	uint8_t expected_result; /* expect value of the register after operation */
	uint8_t flags; /* expected flag values after the operation */
};

void test_add_addc(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;
	int err;
	int i;

	struct arith_testcase add_testcases[] = {
	/* { reg, operand, carry_in, result, flags } */
		{ 0x00, 0x00, 0, 0x00, 0 },
		{ 0x0a, 0x05, 0, 0x0f, 0 },
		{ 0x0a, 0x05, 1, 0x0f, 0 },
		{ 0x0a, 0x06, 0, 0x10, PSW_AC },
		{ 0x11, 0x12, 0, 0x23, 0 },
		{ 0x11, 0x1e, 0, 0x2f, 0 },
		{ 0x11, 0x1e, 1, 0x2f, 0 },
		{ 0x11, 0x1f, 0, 0x30, PSW_AC },
		{ 0x70, 0x0f, 0, 0x7f, 0 },
		{ 0x70, 0x0f, 1, 0x7f, 0 },
		{ 0x70, 0x10, 0, 0x80, PSW_OV },
		{ 0x80, 0x7f, 0, 0xff, 0 },
		{ 0x80, 0x7f, 1, 0xff, 0 },
		{ 0x80, 0x80, 0, 0x00, PSW_C | PSW_OV },
		{ 0xf0, 0x13, 0, 0x03, PSW_C },
	};
	const int add_testcase_count = sizeof(add_testcases)/sizeof(struct arith_testcase);

	struct arith_testcase addc_testcases[] = {
	/* { reg, operand, carry_in, result, flags } */
		{ 0x00, 0x00, 0, 0x00, 0 },
		{ 0x0a, 0x05, 0, 0x0f, 0 },
		{ 0x0a, 0x05, 1, 0x10, PSW_AC },
		{ 0x0a, 0x06, 0, 0x10, PSW_AC },
		{ 0x11, 0x12, 0, 0x23, 0 },
		{ 0x11, 0x1e, 0, 0x2f, 0 },
		{ 0x11, 0x1e, 1, 0x30, PSW_AC },
		{ 0x11, 0x1f, 0, 0x30, PSW_AC },
		{ 0x70, 0x0f, 0, 0x7f, 0 },
		{ 0x70, 0x0f, 1, 0x80, PSW_OV | PSW_AC },
		{ 0x70, 0x10, 0, 0x80, PSW_OV },
		{ 0x80, 0x7f, 0, 0xff, 0 },
		{ 0x80, 0x7f, 1, 0x00, PSW_C | PSW_AC },
		{ 0x80, 0x80, 0, 0x00, PSW_C | PSW_OV },
		{ 0xf0, 0x13, 0, 0x03, PSW_C },
		{ 0x00, 0xff, 1, 0x00, PSW_C | PSW_AC },
		{ 0x00, 0x7f, 1, 0x80, PSW_OV | PSW_AC },
		{ 0x80, 0xff, 0, 0x7f, PSW_C | PSW_OV },
		{ 0x80, 0xff, 1, 0x80, PSW_C | PSW_AC },
	};
	const int addc_testcase_count = sizeof(addc_testcases)/sizeof(struct arith_testcase);

	/* TODO
	 * The following test routines contain lots of duplicate code. It is better
	 * to refactor them into macros or functions.
	 */

	/* ADD  A, #data (opcode = 0x24) */
	for (i = 0; i < add_testcase_count; i++) {
		struct arith_testcase *t = &add_testcases[i];
		ACC(m) = t->reg;
		PSW(m) = 0xff;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR2(0x24, t->operand), data);
		assert_int_equal(err, 0);
		assert_int_equal(ACC(m), t->expected_result);
		assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
		assert_emu51_callbacks(data, CB_SFR_UPDATE);
	}

	/* ADDC A, #data (opcode = 0x34) */
	for (i = 0; i < addc_testcase_count; i++) {
		struct arith_testcase *t = &addc_testcases[i];
		ACC(m) = t->reg;
		if (!t->carry_in)
			PSW(m) = 0;
		else
			PSW(m) = PSW_C;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR2(0x34, t->operand), data);
		assert_int_equal(err, 0);
		assert_int_equal(ACC(m), t->expected_result);
		assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
		assert_emu51_callbacks(data, CB_SFR_UPDATE);
	}

	/* ADD A, iram addr (opcode = 0x25) */
	for (i = 0; i < add_testcase_count; i++) {
		struct arith_testcase *t = &add_testcases[i];
		uint8_t addr;

		/* addr < 128 */
		addr = 0x34;
		ACC(m) = t->reg;
		PSW(m) = 0xff;
		m->iram_lower[addr] = t->operand;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR2(0x25, addr), data);
		assert_int_equal(err, 0);
		assert_int_equal(ACC(m), t->expected_result);
		assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
		assert_emu51_callbacks(data, CB_SFR_UPDATE);

		/* addr >= 128 */
		addr = 0xf0;
		ACC(m) = t->reg;
		PSW(m) = 0xff;
		m->sfr[addr - 0x80] = t->operand;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR2(0x25, addr), data);
		assert_int_equal(err, 0);
		assert_int_equal(ACC(m), t->expected_result);
		assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
		assert_emu51_callbacks(data, CB_SFR_UPDATE);
	}

	/* ADDC A, iram addr (opcode = 0x35) */
	for (i = 0; i < addc_testcase_count; i++) {
		struct arith_testcase *t = &addc_testcases[i];
		uint8_t addr;

		/* addr < 128 */
		addr = 0x34;
		ACC(m) = t->reg;
		PSW(m) = t->carry_in ? 0xff : ~PSW_C;
		m->iram_lower[addr] = t->operand;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR2(0x35, addr), data);
		assert_int_equal(err, 0);
		assert_int_equal(ACC(m), t->expected_result);
		assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
		assert_emu51_callbacks(data, CB_SFR_UPDATE);

		/* addr >= 128 */
		addr = 0xf0;
		ACC(m) = t->reg;
		PSW(m) = t->carry_in ? 0xff : ~PSW_C;
		m->sfr[addr - 0x80] = t->operand;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR2(0x35, addr), data);
		assert_int_equal(err, 0);
		assert_int_equal(ACC(m), t->expected_result);
		assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
		assert_emu51_callbacks(data, CB_SFR_UPDATE);
	}

	/* ADD A, @R0 (opcode = 0x26)
	 * ADD A, @R1 (opcode = 0x27)
	 */
	for (i = 0; i < add_testcase_count; i++) {
		struct arith_testcase *t = &add_testcases[i];
		int reg;

		for (reg = 0; reg <= 1; reg++) {
			uint8_t opcode = 0x26 + reg;
			uint8_t addr;

			/* addr < 128 */
			addr = 0x30;
			ACC(m) = t->reg;
			PSW(m) = 0xff;
			R_REG(m, reg) = addr;
			m->iram_lower[addr] = t->operand;
			expect_value(callback_sfr_update, index, SFR_PSW);
			err = run_instr(INSTR1(opcode), data);
			assert_int_equal(err, 0);
			assert_int_equal(ACC(m), t->expected_result);
			assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
			assert_emu51_callbacks(data, CB_SFR_UPDATE);

			/* addr >= 128 */
			addr = 0xf0;
			ACC(m) = t->reg;
			PSW(m) = 0xff;
			R_REG(m, reg) = addr;
			m->iram_upper[addr - 0x80] = t->operand;
			expect_value(callback_sfr_update, index, SFR_PSW);
			err = run_instr(INSTR1(opcode), data);
			assert_int_equal(err, 0);
			assert_int_equal(ACC(m), t->expected_result);
			assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
			assert_emu51_callbacks(data, CB_SFR_UPDATE);
		}
	}

	/* ADDC A, @R0 (opcode = 0x36)
	 * ADDC A, @R1 (opcode = 0x37)
	 */
	for (i = 0; i < addc_testcase_count; i++) {
		struct arith_testcase *t = &addc_testcases[i];
		int reg;

		for (reg = 0; reg <= 1; reg++) {
			uint8_t opcode = 0x36 + reg;
			uint8_t addr;

			/* addr < 128 */
			addr = 0x30;
			ACC(m) = t->reg;
			PSW(m) = t->carry_in ? PSW_C : 0;
			R_REG(m, reg) = addr;
			m->iram_lower[addr] = t->operand;
			expect_value(callback_sfr_update, index, SFR_PSW);
			err = run_instr(INSTR1(opcode), data);
			assert_int_equal(err, 0);
			assert_int_equal(ACC(m), t->expected_result);
			assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
			assert_emu51_callbacks(data, CB_SFR_UPDATE);

			/* addr >= 128 */
			addr = 0xf0;
			ACC(m) = t->reg;
			PSW(m) = t->carry_in ? PSW_C : 0;
			R_REG(m, reg) = addr;
			m->iram_upper[addr - 0x80] = t->operand;
			expect_value(callback_sfr_update, index, SFR_PSW);
			err = run_instr(INSTR1(opcode), data);
			assert_int_equal(err, 0);
			assert_int_equal(ACC(m), t->expected_result);
			assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
			assert_emu51_callbacks(data, CB_SFR_UPDATE);
		}
	}

	/* ADD A, Rn (opcode = 0x28~0x2f) */
	for (i = 0; i < add_testcase_count; i++) {
		struct arith_testcase *t = &add_testcases[i];
		int reg;

		for (reg = 0; reg <= 7; reg++) {
			uint8_t opcode = 0x28 + reg;

			/* addr < 128 */
			ACC(m) = t->reg;
			PSW(m) = 0xff;
			R_REG(m, reg) = t->operand;
			expect_value(callback_sfr_update, index, SFR_PSW);
			err = run_instr(INSTR1(opcode), data);
			assert_int_equal(err, 0);
			assert_int_equal(ACC(m), t->expected_result);
			assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
			assert_emu51_callbacks(data, CB_SFR_UPDATE);
		}
	}

	/* ADDC A, Rn (opcode = 0x38~0x3f) */
	for (i = 0; i < addc_testcase_count; i++) {
		struct arith_testcase *t = &addc_testcases[i];
		int reg;

		for (reg = 0; reg <= 1; reg++) {
			uint8_t opcode = 0x38 + reg;

			ACC(m) = t->reg;
			PSW(m) = t->carry_in ? PSW_C : 0;
			R_REG(m, reg) = t->operand;
			expect_value(callback_sfr_update, index, SFR_PSW);
			err = run_instr(INSTR1(opcode), data);
			assert_int_equal(err, 0);
			assert_int_equal(ACC(m), t->expected_result);
			assert_int_equal(PSW(m) & (PSW_AC | PSW_OV | PSW_C), t->flags);
			assert_emu51_callbacks(data, CB_SFR_UPDATE);
		}
	}

	free_test_data(data);
}

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_add_addc),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
