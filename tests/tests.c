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
	uint8_t iram[256];

	/* fill the memory with arbitrary data to see if reset really works */
	for (i = 0; i < sizeof(iram); i++)
		iram[i] = 0xaa;

	/* initialize the emulator struct */
	memset(&m, 0, sizeof(m));
	m.iram = iram;
	m.iram_len = sizeof(iram);
	m.pc = 0x10; /* set pc to arbitrary value to test the reset */

	emu51_reset(&m);
	assert_int_equal(m.pc, 0);
	assert_int_equal(m.iram[ADDR_SP], 0x07); /* initial value of SP is 07h */

}

void test_instr_table(void **state)
{
	int opcode;
	for (opcode = 0; opcode <= 255; opcode++) {
		/* The instruction table is a lookup table indexed by opcode. */
		const emu51_instr *instr = emu51_get_instr(opcode);

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
