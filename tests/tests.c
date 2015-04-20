#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include <emu51.h>

/* library internal headers */
#include <instr.h>

/* use cmocka's version of memory allocation functions with checks */
#define malloc test_malloc
#define calloc test_calloc
#define free test_free

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

	emu51_reset(&m);
	assert_int_equal(m.pc, 0);
	assert_int_equal(m.sfr[SFR_SP], 0x07); /* initial value of SP is 07h */

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

void test_step(void **state)
{
	uint8_t iram_lower[128], sfr[128];
	uint8_t *pmem = calloc(4096, 1);
	uint8_t *xram = calloc(4096, 1);
	int error, cycles;

	/* create an emulator with 128-bytes of iram, 4K of xram and 4K of rom */
	emu51 m;
	memset(&m, 0, sizeof(m));
	m.pmem = pmem;
	m.pmem_len = 4096;
	m.sfr = sfr;
	m.iram_lower = iram_lower;
	m.xram = xram;
	m.xram_len = 4096;

	/* pc should advance on each NOP step */
	pmem[0] = pmem[1] = 0x00; /* NOP */
	m.pc = 0;
	error = emu51_step(&m, &cycles);
	assert_int_equal(error, 0);
	assert_int_equal(cycles, 1);
	assert_int_equal(m.pc, 1);
	error = emu51_step(&m, &cycles);
	assert_int_equal(error, 0);
	assert_int_equal(cycles, 1);
	assert_int_equal(m.pc, 2);

	/* should not execute instruction outside program memory boundary */
	pmem[4095] = 0x00; /* NOP */
	m.pc = 4095; /* this one is OK */
	error = emu51_step(&m, NULL);
	assert_int_equal(error, 0);
	assert_int_equal(m.pc, 4096);
	error = emu51_step(&m, NULL); /* pc == 4096, this is not OK */
	assert_int_equal(error, EMU51_PMEM_OUT_OF_RANGE);

	/* 2-byte instruction on byte 4094:4095 is OK */
	pmem[4094] = 0x01; /* AJMP, 2-byte instruction */
	m.pc = 4094;
	error = emu51_step(&m, NULL);
	assert_int_equal(error, 0);

	/* 2-byte instruction on byte 4095:4096 is not OK */
	pmem[4095] = 0x01; /* AJMP */
	m.pc = 4095;
	error = emu51_step(&m, NULL);
	assert_int_equal(error, EMU51_PMEM_OUT_OF_RANGE);

	free(pmem);
	free(xram);
}

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_reset),
		cmocka_unit_test(test_instr_table),
		cmocka_unit_test(test_step),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
