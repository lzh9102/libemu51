/* common macro/functions for testing instructions */

#ifndef _TEST_INSTR_COMMON_H_
#define _TEST_INSTR_COMMON_H_

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>
#include <stdlib.h>

#include <emu51.h>

/* library internal headers */
#include <instr.h>

/* Replace memory allocation functions with cmocka's version, which can detect
 * memory leak and buffer overflow bugs.
 */
#define malloc test_malloc
#define calloc test_calloc
#define free test_free

#define PMEM_SIZE 4096         /* program memory size used in tests */
#define XRAM_SIZE (32 * 1024)  /* external memory size used in tests */

#define ACC(m) m->sfr[SFR_ACC]
#define PSW(m) m->sfr[SFR_PSW]
#define SP(m) m->sfr[SFR_SP]
#define PC(m) m->pc

#define R_REG_BASE(m) (m->sfr[SFR_PSW] & (PSW_RS1 | PSW_RS0))
#define R_REG(m, n) m->iram_lower[R_REG_BASE(m) + (n)]
#define R0(m) R_REG(m, 0)
#define R1(m) R_REG(m, 1)
#define R2(m) R_REG(m, 2)
#define R3(m) R_REG(m, 3)
#define R4(m) R_REG(m, 4)
#define R5(m) R_REG(m, 5)
#define R6(m) R_REG(m, 6)
#define R7(m) R_REG(m, 7)

static void iram_write(emu51 *m, uint8_t addr, uint8_t value)
{
	if (addr < 0x80)
		m->iram_lower[addr] = value;
	else
		m->iram_upper[addr - 0x80] = value;
}

static uint8_t iram_read(emu51 *m, uint8_t addr)
{
	if (addr < 0x80)
		return m->iram_lower[addr];
	else
		return m->iram_upper[addr - 0x80];
}

/* data used in unit tests */
typedef struct testdata
{
	/* each buffer is allocated separately to detect buffer overflow */
	emu51 *m; /* the emulator struct */
	uint8_t *sfr; /* buffer to hold SFRs, 128 bytes */
	uint8_t *pmem; /* program memory */
	uint8_t *iram_lower; /* lower internal RAM, 128 bytes */
	uint8_t *iram_upper; /* lower internal RAM, 128 bytes */
	uint8_t *xram; /* external RAM */

	/* Bitmap represent if each callback is called. Each bit represents a
	 * callback. */
#define CB_SFR_UPDATE  (1<<0)
#define CB_IRAM_UPDATE (1<<1)
#define CB_XRAM_UPDATE (1<<2)
#define CB_IO_WRITE (1<<3)
#define CB_IO_READ (1<<3)
	uint16_t callback_called;
} testdata;

/* callback handlers */

static void callback_sfr_update(emu51 *m, uint8_t index)
{
	testdata *data = m->userdata;
	data->callback_called |= CB_SFR_UPDATE;
	check_expected(index);
}

static void callback_iram_update(emu51 *m, uint8_t addr)
{
	testdata *data = m->userdata;
	data->callback_called |= CB_IRAM_UPDATE;
	check_expected(addr);
}

static void callback_xram_update(emu51 *m, uint16_t addr)
{
	testdata *data = m->userdata;
	data->callback_called |= CB_XRAM_UPDATE;
	check_expected(addr);
}

static void callback_io_write(emu51 *m, uint8_t portno, uint8_t bitmask, uint8_t data)
{
	testdata *td = m->userdata;
	td->callback_called |= CB_IO_WRITE;
	check_expected(portno);
	check_expected(bitmask);
	check_expected(data);
}

static void callback_io_read(emu51 *m, uint8_t portno, uint8_t bitmask, uint8_t *data)
{
	testdata *td = m->userdata;
	td->callback_called |= CB_IO_READ;
	check_expected(portno);
	check_expected(bitmask);
	check_expected(data);
}

/* Create emulator and buffers for testing.
 *
 * Do not share the created test data across tests because this will make it
 * difficult to locate the test that caused memory bugs. Instead, allocate the
 * data in each test and free it after use.
 */
static testdata *alloc_test_data()
{
	testdata *data = calloc(1, sizeof(testdata));

	/* emulator struct */
	data->m = calloc(1, sizeof(emu51));

	/* special function register */
	data->sfr = calloc(128, 1);

	/* internal RAM */
	data->iram_lower = calloc(128, 1);
	data->iram_upper = calloc(128, 1);

	/* 4KB of program memory */
	data->pmem = calloc(PMEM_SIZE, 1);

	/* external RAM */
	data->xram = calloc(XRAM_SIZE, 1);

	/* setup the emulator */
	memset(data->m, 0, sizeof(emu51));
	data->m->sfr = data->sfr;
	data->m->pmem = data->pmem;
	data->m->pmem_len = PMEM_SIZE;
	data->m->iram_lower = data->iram_lower;
	data->m->iram_upper = data->iram_upper;
	data->m->xram = data->xram;
	data->m->xram_len = XRAM_SIZE;
	emu51_reset(data->m);

	/* callbacks */
	emu51 *m = data->m;
	m->callback.sfr_update = callback_sfr_update;
	m->callback.iram_update = callback_iram_update;
	m->callback.xram_update = callback_xram_update;
	m->callback.io_write = callback_io_write;
	m->callback.io_read = callback_io_read;

	data->m->userdata = data; /* make it possible to find data from m */

	return data;
}

/* Duplicate the data (make a separate copy of it).
 *
 * It's the caller's responsibility to free the duplicated data.
 */
static testdata *dup_test_data(const testdata *orig)
{
	testdata *data = alloc_test_data();

	/* copy the emulator fields
	 * ATTENTION: pointers like pmem and iram are overwritten by the pointers
	 * in the orig struct. It is necessary to fix this.
	 */
	*data->m = *orig->m;
	/* fix the pointer fields */
	data->m->sfr = data->sfr;
	data->m->pmem = data->pmem;
	data->m->iram_lower = data->iram_lower;
	data->m->iram_upper = data->iram_upper;
	data->m->xram = data->xram;
	data->m->userdata = data;

	/* copy the content of buffers */
	memcpy(data->sfr, orig->sfr, 128);
	memcpy(data->pmem, orig->pmem, PMEM_SIZE);
	memcpy(data->iram_lower, orig->iram_lower, 128);
	memcpy(data->iram_upper, orig->iram_upper, 128);
	memcpy(data->xram, orig->m->xram, XRAM_SIZE);

	return data;
}

/* free test data allocated by alloc_test_data() */
static void free_test_data(testdata *data)
{
	free(data->m);
	free(data->sfr);
	free(data->pmem);
	free(data->iram_lower);
	free(data->iram_upper);
	free(data->xram);
	free(data);
}

/* write random data to the buffer */
static void write_random_data(uint8_t *buffer, size_t size)
{
	unsigned int i;
	for (i = 0; i < size; i++)
		buffer[i] = rand() & 0xff;
}

/* write random data to the memories (pmem, iram, xram) in the testdata */
static void write_random_data_to_memories(testdata *data)
{
	write_random_data(data->pmem, PMEM_SIZE);
	write_random_data(data->sfr, 128);
	write_random_data(data->iram_lower, 128);
	write_random_data(data->iram_upper, 128);
	write_random_data(data->xram, XRAM_SIZE);
}

/* get the upper 8-bit of a 16-bit word */
#define UPPER_BYTE(word) (((word)>>8) & 0xff)
/* get the lower 8-bit of a 16-bit word */
#define LOWER_BYTE(word) ((word) & 0xff)

#define GET_DPTR(m) ((m->sfr[SFR_DPH] << 8) | m->sfr[SFR_DPL])
#define SET_DPTR(m, dptr) do { \
	m->sfr[SFR_DPL] = LOWER_BYTE(dptr); \
	m->sfr[SFR_DPH] = UPPER_BYTE(dptr); } while (0)

#define assert_emu51_sfr_equal(data1, data2) \
	assert_memory_equal(data1->sfr, data2->sfr, 128)

#define assert_emu51_iram_equal(data1, data2) do { \
	assert_memory_equal(data1->iram_lower, data2->iram_lower, 128); \
	assert_memory_equal(data1->iram_upper, data2->iram_upper, 128); } while (0)

#define assert_emu51_xram_equal(data1, data2) \
	assert_memory_equal(data1->xram, data2->xram, XRAM_SIZE)

/* sfr, iram, xram all equal */
#define assert_emu51_all_ram_equal(data1, data2) do { \
	assert_emu51_sfr_equal(data1, data2); \
	assert_emu51_iram_equal(data1, data2); \
	assert_emu51_xram_equal(data1, data2); } while (0)

#define assert_emu51_callbacks(data, callback_bits) \
	assert_int_equal(data->callback_called, callback_bits)

#define assert_emu51_flags_changed(data1, data2, psw_mask) \
	assert_int_equal((data1)->sfr[SFR_PSW] ^ (data2)->sfr[SFR_PSW], psw_mask)

#define assert_emu51_flags_set(data, psw_mask) \
	assert_int_equal((data)->sfr[SFR_PSW] & (psw_mask), psw_mask)

#define assert_emu51_flags_not_set(data, psw_mask) \
	assert_int_equal((data)->sfr[SFR_PSW] & (psw_mask), 0)

/* the highest byte (0xff000000) is used to store instruction length */
#define INSTR1(opcode) ((0x01 << 24) | (opcode))
#define INSTR2(opcode, op1) ((0x02 << 24) | ((op1) << 8) | (opcode))
#define INSTR3(opcode, op1, op2) ((0x03 << 24) | \
		((op2) << 16) | ((op1) << 8) | (opcode))

/* put the instruction in pc and run it */
static int run_instr(uint32_t instr, testdata *data)
{
	/* reset callback */
	data->callback_called = 0;

	uint8_t instr_length = (instr >> 24) & 0xff;
	uint8_t buffer[3];

	buffer[0] = instr & 0xff;
	buffer[1] = (instr >> 8) & 0xff;
	buffer[2] = (instr >> 16) & 0xff;

	uint8_t opcode = buffer[0];

	/* get instruction information */
	const emu51_instr *instr_info = _emu51_decode_instr(opcode);
	assert_int_equal(instr_info->bytes, instr_length);
	assert_non_null(instr_info->handler);

	/* Run the instruction on an emulator without callback functions. */
	testdata *data_no_callback = dup_test_data(data); /* clone emulator */
	memset(&data_no_callback->m->callback, 0, sizeof(emu51_callbacks)); /* clear callbacks */
	instr_info->handler(instr_info, buffer, data_no_callback->m); /* run instruction */
	free_test_data(data_no_callback); /* free the cloned emulator */

	return instr_info->handler(instr_info, buffer, data->m);
}

#endif /* _TEST_INSTR_COMMON_H_ */
