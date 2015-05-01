/* tests for memory-accessing functions */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>
#include <stdlib.h>

#include <emu51.h>
#include <mem.h>

int setup(void **state)
{
	/* create emu51 instance */
	emu51 *m = calloc(1, sizeof(emu51));
	memset(m, 0, sizeof(emu51));

	/* memory buffers */
	m->pmem = calloc(4096, 1);
	m->pmem_len = 4096;
	m->iram_lower = calloc(128, 1);
	m->iram_upper = calloc(128, 1);
	m->sfr = calloc(128, 1);
	m->xram = calloc(65536, 1);
	m->xram_len = 65536;

	*state = m;
	return 0;
}

int teardown(void **state)
{
	emu51 *m = *state;
	if (m->pmem)
		free((void*)m->pmem);
	if (m->iram_lower)
		free(m->iram_lower);
	if (m->iram_upper)
		free(m->iram_upper);
	if (m->sfr)
		free(m->sfr);
	if (m->xram)
		free(m->xram);
	return 0;
}

void test_direct_addr_read(void **state)
{
	emu51 *m = *state;

	/* 0 <= address < 0x80 is iram */
	m->iram_lower[0] = 0xaa;
	assert_int_equal(direct_addr_read(m, 0), 0xaa);
	m->iram_lower[0x7f] = 0xab;
	assert_int_equal(direct_addr_read(m, 0x7f), 0xab);

	/* address > 0x80 is SFR */
	m->iram_upper[0] = 0xac;
	m->sfr[0] = 0xad;
	assert_int_equal(direct_addr_read(m, 0x80), 0xad);
	m->iram_upper[0x7f] = 0xae;
	m->sfr[0x7f] = 0xaf;
	assert_int_equal(direct_addr_read(m, 0xff), 0xaf);
}

void test_direct_addr_write(void **state)
{
	emu51 *m = *state;

	/* 0 <= address < 0x80: iram */
	direct_addr_write(m, 0, 0xaa);
	assert_int_equal(m->iram_lower[0], 0xaa);
	assert_int_equal(m->iram_lower[1], 0x00);
	direct_addr_write(m, 0x7f, 0xab);
	assert_int_equal(m->iram_lower[0x7f], 0xab);
	assert_int_equal(m->iram_lower[0x7e], 0x00);
	assert_int_equal(m->iram_upper[0], 0x00);
	assert_int_equal(m->sfr[0], 0x00);

	/* address > 0x80: SFR */
	direct_addr_write(m, 0x80, 0xc0);
	assert_int_equal(m->sfr[0], 0xc0);
	assert_int_equal(m->iram_upper[0], 0x00);
	direct_addr_write(m, 0xff, 0xc1);
	assert_int_equal(m->sfr[0x7f], 0xc1);
	assert_int_equal(m->iram_upper[0x7f], 0x00);
}

void test_indirect_addr_read(void **state)
{
	emu51 *m = *state;
	uint8_t data;
	int err;

	/* address < 0x80, *address < 0x80: dereference iram, read iram_lower */
	m->iram_lower[0x10] = 0x20;
	m->iram_lower[0x20] = 0xf1; /* 0x20 => iram_lower[0x20] */
	err = indirect_addr_read(m, 0x10, &data);
	assert_int_equal(err, 0);
	assert_int_equal(data, 0xf1);

	/* address < 0x80, *address >= 0x80: dereference iram, read iram_upper */
	m->iram_lower[0x30] = 0x8f;
	m->iram_upper[0x0f] = 0xf2; /* 0x8f => iram_upper[0x0f] */
	err = indirect_addr_read(m, 0x30, &data);
	assert_int_equal(err, 0);
	assert_int_equal(data, 0xf2);

	/* address >= 0x80, *address < 0x80: dereference SFR, read iram_lower */
	m->sfr[SFR_ACC] = 0x50;
	m->iram_lower[0x50] = 0xf3; /* 0x50 => iram_lower[0x50] */
	err = indirect_addr_read(m, SFR_ACC + SFR_BASE_ADDR, &data);
	assert_int_equal(err, 0);
	assert_int_equal(data, 0xf3);

	/* address >= 0x80, *address >= 0x80: dereferenc SFR, read iram_upper */
	m->sfr[SFR_SP] = 0x9f;
	m->iram_upper[0x1f] = 0xf4; /* 0x9f => iram_upper[0x1f] */
	err = indirect_addr_read(m, SFR_SP + SFR_BASE_ADDR, &data);
	assert_int_equal(err, 0);
	assert_int_equal(data, 0xf4);

	/* delete iram_upper to test for error conditions */
	free(m->iram_upper);
	m->iram_upper = NULL;

	/* *address < 0x80 */
	m->sfr[SFR_B] = 0x7f;
	m->iram_lower[0x7f] = 0xf5;
	err = indirect_addr_read(m, SFR_B + SFR_BASE_ADDR, &data);
	assert_int_equal(err, 0);
	assert_int_equal(data, 0xf5);

	/* *address >= 0x80: error */
	m->sfr[SFR_B] = 0x80;
	err = indirect_addr_read(m, SFR_B + SFR_BASE_ADDR, &data);
	assert_int_equal(err, EMU51_IRAM_OUT_OF_RANGE);
}

void test_indirect_addr_write(void **state)
{
	emu51 *m = *state;
	int err;

	/* address < 0x80, *address < 0x80: dereference iram, write iram_lower */
	m->iram_lower[0x10] = 0x20;
	err = indirect_addr_write(m, 0x10, 0xf1);
	assert_int_equal(err, 0);
	assert_int_equal(m->iram_lower[0x20], 0xf1); /* 0x20 -> iram_lower[0x20] */

	/* address < 0x80, *address >= 0x80: dereference iram, read iram_upper */
	m->iram_lower[0x30] = 0x8f;
	err = indirect_addr_write(m, 0x30, 0xf2);
	assert_int_equal(err, 0);
	assert_int_equal(m->iram_upper[0x0f], 0xf2); /* 0x8f => iram_upper[0x0f] */

	/* address >= 0x80, *address < 0x80: dereference SFR, read iram_lower */
	m->sfr[SFR_ACC] = 0x50;
	err = indirect_addr_write(m, SFR_ACC + SFR_BASE_ADDR, 0xf3);
	assert_int_equal(err, 0);
	assert_int_equal(m->iram_lower[0x50], 0xf3); /* 0x50 => iram_lower[0x50] */

	/* address >= 0x80, *address >= 0x80: dereferenc SFR, read iram_upper */
	m->sfr[SFR_SP] = 0x9f;
	err = indirect_addr_write(m, SFR_SP + SFR_BASE_ADDR, 0xf4);
	assert_int_equal(err, 0);
	assert_int_equal(m->iram_upper[0x1f], 0xf4); /* 0x9f => iram_upper[0x1f] */

	/* delete iram_upper to test for error conditions */
	free(m->iram_upper);
	m->iram_upper = NULL;

	/* *address < 0x80 */
	m->sfr[SFR_B] = 0x7f;
	err = indirect_addr_write(m, SFR_B + SFR_BASE_ADDR, 0xf5);
	assert_int_equal(err, 0);
	assert_int_equal(m->iram_lower[0x7f], 0xf5); /* 0x7f => iram_lower[0x7f] */

	/* *address >= 0x80: error */
	m->sfr[SFR_B] = 0x80;
	err = indirect_addr_write(m, SFR_B + SFR_BASE_ADDR, 0xf6);
	assert_int_equal(err, EMU51_IRAM_OUT_OF_RANGE);
	assert_int_equal(m->sfr[0], 0x00); /* make sure sfr is not clobbered */
}

void test_stack_push(void **state)
{
	emu51 *m = *state;
	int err;

	/* SP < 0x7f: increment SP and write iram_lower */
	m->sfr[SFR_SP] = 0x7e;
	m->iram_lower[0x7e] = 0x00;
	m->iram_lower[0x7f] = 0x00;
	m->iram_upper[0x00] = 0x00;
	err = stack_push(m, 0xf1);
	assert_int_equal(m->sfr[SFR_SP], 0x7f);
	assert_int_equal(m->iram_lower[0x7e], 0x00);
	assert_int_equal(m->iram_lower[0x7f], 0xf1);
	assert_int_equal(m->iram_upper[0x00], 0x00);

	/* SP >= 0x7f: increment SP and write iram_upper */
	m->sfr[SFR_SP] = 0x7f;
	m->iram_lower[0x7e] = 0x00;
	m->iram_lower[0x7f] = 0x00;
	m->iram_upper[0x00] = 0x00;
	err = stack_push(m, 0xf2);
	assert_int_equal(m->sfr[SFR_SP], 0x80);
	assert_int_equal(m->iram_lower[0x7e], 0x00);
	assert_int_equal(m->iram_lower[0x7f], 0x00);
	assert_int_equal(m->iram_upper[0x00], 0xf2);

	/* delete iram_upper to test for error conditions */
	free(m->iram_upper);
	m->iram_upper = NULL;

	/* SP < 0x7f: ok */
	m->sfr[SFR_SP] = 0x7e;
	m->iram_lower[0x7e] = 0x00;
	m->iram_lower[0x7f] = 0x00;
	err = stack_push(m, 0xf3);
	assert_int_equal(m->sfr[SFR_SP], 0x7f);
	assert_int_equal(m->iram_lower[0x7e], 0x00);
	assert_int_equal(m->iram_lower[0x7f], 0xf3);

	/* SP >= 0x7f: error */
	m->sfr[SFR_SP] = 0x7f;
	err = stack_push(m, 0xf4);
	assert_int_equal(err, EMU51_IRAM_OUT_OF_RANGE);
}

int main()
{
#define TEST_ENTRY(name) cmocka_unit_test_setup_teardown(name, setup, teardown)
	const struct CMUnitTest tests[] = {
		TEST_ENTRY(test_direct_addr_read),
		TEST_ENTRY(test_direct_addr_write),
		TEST_ENTRY(test_indirect_addr_read),
		TEST_ENTRY(test_indirect_addr_write),
		TEST_ENTRY(test_stack_push),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
