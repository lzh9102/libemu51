#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include <emu51.h>

/* library internal headers */
#include <instr.h>

void test_reset(void **state)
{
	int i;
	emu51 m;
	uint8_t sfr[128];

	/* fill the sfr buffer with arbitrary data to see if reset really works */
	for (i = 0; i < sizeof(sfr); i++)
		sfr[i] = 0xaa;

	/* initialize the emulator struct */
	memset(&m, 0, sizeof(m));
	m.sfr = sfr;
	m.pc = 0x10; /* set pc to arbitrary value to test the reset */
	m.errno = -1; /* set errno to arbitrary value */

	emu51_reset(&m);
	assert_int_equal(m.pc, 0);
	assert_int_equal(m.sfr[SFR_SP], 0x07); /* initial value of SP is 07h */
	assert_int_equal(m.errno, 0);

}

void test_instr_table(void **state)
{
	int opcode;
	for (opcode = 0; opcode <= 255; opcode++) {
		/* The instruction table is a lookup table indexed by opcode. */
		const emu51_instr *instr = _emu51_get_instr(opcode);

		/* the opcode-th instruction should have opcode opcode */
		assert_int_equal(instr->opcode, opcode);

		/* if the instruction is not implemented (has zero bytes), the handler
		   should be NULL */
		if (instr->bytes == 0)
			assert_null(instr->handler);
	}
}

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_reset),
		cmocka_unit_test(test_instr_table),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
