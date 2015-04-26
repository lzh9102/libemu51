/* unit tests for the instructions */

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

void iram_write(emu51 *m, uint8_t addr, uint8_t value)
{
	if (addr < 0x80)
		m->iram_lower[addr] = value;
	else
		m->iram_upper[addr - 0x80] = value;
}

uint8_t iram_read(emu51 *m, uint8_t addr)
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

void callback_sfr_update(emu51 *m, uint8_t index)
{
	testdata *data = m->userdata;
	data->callback_called |= CB_SFR_UPDATE;
	check_expected(index);
}

void callback_iram_update(emu51 *m, uint8_t addr)
{
	testdata *data = m->userdata;
	data->callback_called |= CB_IRAM_UPDATE;
	check_expected(addr);
}

void callback_xram_update(emu51 *m, uint16_t addr)
{
	testdata *data = m->userdata;
	data->callback_called |= CB_XRAM_UPDATE;
	check_expected(addr);
}

void callback_io_write(emu51 *m, uint8_t portno, uint8_t bitmask, uint8_t data)
{
	testdata *td = m->userdata;
	td->callback_called |= CB_IO_WRITE;
	check_expected(portno);
	check_expected(bitmask);
	check_expected(data);
}

void callback_io_read(emu51 *m, uint8_t portno, uint8_t bitmask, uint8_t *data)
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
testdata *alloc_test_data()
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
testdata *dup_test_data(const testdata *orig)
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
void free_test_data(testdata *data)
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
void write_random_data(uint8_t *buffer, size_t size)
{
	int i;
	for (i = 0; i < size; i++)
		buffer[i] = rand() & 0xff;
}

/* write random data to the memories (pmem, iram, xram) in the testdata */
void write_random_data_to_memories(testdata *data)
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
int run_instr(uint32_t instr, testdata *data)
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

/* here goes the test functions */

void test_nop(void **state)
{
	testdata *data = alloc_test_data();

	/* fill in random data to emulator memory */
	write_random_data_to_memories(data);

	/* fill in NOP in pmem[0] */
	data->pmem[0] = 0x00;

	/* save the original state */
	testdata *orig_data = dup_test_data(data);

	run_instr(INSTR1(0x00), data);

	/* nop shouldn't change any of the SFRs, iram or xram */
	assert_emu51_all_ram_equal(data, orig_data);

	/* nop shouldn't trigger any callbacks */
	assert_emu51_callbacks(data, 0);

	free_test_data(orig_data);
	free_test_data(data);
}

void test_ajmp(void **state)
{
	testdata *data = alloc_test_data();

	write_random_data_to_memories(data);

	testdata *orig_data = dup_test_data(data);

#define TEST_AJMP(op_, pc_, page_, second_byte_, target_) do { \
	data->m->pc = pc_; \
	run_instr(INSTR2(op_, second_byte_), data); \
	assert_emu51_all_ram_equal(data, orig_data); \
	assert_int_equal(data->m->pc, target_); \
	assert_emu51_callbacks(data, 0); } while (0)

	/* AJMP page0 (0x01)
	 *
	 * pc:     1010 1111 1010 1010 (0xafaa)
	 * page:         000           (0x0)
	 * second byte:      0101 0101 (0x55)
	 * target: 1010 1000 0101 0101 (0xa855)
	 */
	TEST_AJMP(0x01, 0xafaa, 0x0, 0x55, 0xa855);

	/* AJMP page1 (0x21)
	 *
	 * pc:     1010 1110 1010 1010 (0xaeaa)
	 * page:         001           (0x1)
	 * second byte:      0101 0101 (0x55)
	 * target: 1010 1001 0101 0101 (0xa955)
	 */
	TEST_AJMP(0x21, 0xaeaa, 0x1, 0x55, 0xa955);

	/* AJMP page2 (0x41)
	 *
	 * pc:     1010 1101 1010 1010 (0xadaa)
	 * page:         010           (0x2)
	 * second byte:      0101 0101 (0x55)
	 * target: 1010 1010 0101 0101 (0xaa55)
	 */
	TEST_AJMP(0x41, 0xadaa, 0x2, 0x55, 0xaa55);

	/* AJMP page3 (0x61)
	 *
	 * pc:     1010 1100 1010 1010 (0xacaa)
	 * page:         011           (0x3)
	 * second byte:      0101 0101 (0x55)
	 * target: 1010 1011 0101 0101 (0xab55)
	 */
	TEST_AJMP(0x61, 0xacaa, 0x3, 0x55, 0xab55);

	/* AJMP page4 (0x81)
	 *
	 * pc:     1010 1011 1010 1010 (0xabaa)
	 * page:         100           (0x4)
	 * second byte:      0101 0101 (0x55)
	 * target: 1010 1100 0101 0101 (0xac55)
	 */
	TEST_AJMP(0x81, 0xabaa, 0x4, 0x55, 0xac55);

	/* AJMP page5 (0xa1)
	 *
	 * pc:     1010 1010 1010 1010 (0xaaaa)
	 * page:         101           (0x5)
	 * second byte:      0101 0101 (0x55)
	 * target: 1010 1101 0101 0101 (0xad55)
	 */
	TEST_AJMP(0xa1, 0xaaaa, 0x5, 0x55, 0xad55);

	/* AJMP page6 (0xc1)
	 *
	 * pc:     1010 1001 1010 1010 (0xa9aa)
	 * page:         110           (0x6)
	 * second byte:      0101 0101 (0x55)
	 * target: 1010 1110 0101 0101 (0xae55)
	 */
	TEST_AJMP(0xc1, 0xa9aa, 0x6, 0x55, 0xae55);

	/* AJMP page7 (0xe1)
	 *
	 * pc:     1010 1000 1010 1010 (0xa8aa)
	 * page:         111           (0x7)
	 * second byte:      0101 0101 (0x55)
	 * target: 1010 1111 0101 0101 (0xaf55)
	 */
	TEST_AJMP(0xe1, 0xa8aa, 0x7, 0x55, 0xaf55);

#undef TEST_AJMP

	free_test_data(orig_data);
	free_test_data(data);
}

void test_jmp(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;

	write_random_data_to_memories(data);

	/* jmp */
	uint8_t opcode = 0x73;
	uint16_t dptr = 0x1234;
	uint8_t acc = 32;
	data->pmem[0] = opcode;
	m->sfr[SFR_ACC] = acc;
	m->sfr[SFR_DPL] = LOWER_BYTE(dptr);
	m->sfr[SFR_DPH] = UPPER_BYTE(dptr);

	testdata *orig_data = dup_test_data(data);

	int err = run_instr(INSTR1(0x73), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, dptr + acc);
	assert_emu51_all_ram_equal(data, orig_data);
	assert_emu51_callbacks(data, 0);

	free_test_data(data);
	free_test_data(orig_data);
}

void test_ljmp(void **state)
{
	testdata *data = alloc_test_data();

	write_random_data_to_memories(data);

	/* save the original state */
	testdata *orig_data = dup_test_data(data);

	/* ljmp */
	uint8_t opcode = 0x02;
	uint16_t target_addr = 0x1234;
	int err = run_instr(
			INSTR3(opcode, UPPER_BYTE(target_addr), LOWER_BYTE(target_addr)),
			data);
	assert_int_equal(err, 0);

	/* ljmp shouldn't change any of the memories */
	assert_emu51_all_ram_equal(data, orig_data);

	/* ljmp should set the pc to the target address */
	assert_int_equal(data->m->pc, target_addr);

	/* no callback is called */
	assert_emu51_callbacks(data, 0);

	free_test_data(orig_data);
	free_test_data(data);
}

void test_sjmp(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;

	write_random_data_to_memories(data);

	/* save the original state */
	testdata *orig_data = dup_test_data(data);

	/* sjmp forward */
	uint8_t opcode = 0x80;
	uint8_t reladdr = 127; /* signed 127 */
	m->pc = 0;
	int err = run_instr(INSTR2(opcode, reladdr), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 127);
	assert_emu51_all_ram_equal(data, orig_data);
	assert_emu51_callbacks(data, 0);

	/* sjmp backwards */
	opcode = 0x80;
	reladdr = 0x80; /* signed -128 */
	m->pc = 254;
	err = run_instr(INSTR2(opcode, reladdr), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 254 - 128);
	assert_emu51_callbacks(data, 0);

	free_test_data(orig_data);
	free_test_data(data);
}

void test_movc()
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

void test_cjne(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;
	uint8_t opcode = 0, addr;
	int err = 0;

	/* CJNE A, #data, reladdr (opcode=0xb4) */
	opcode = 0xb4;
	/* A < #data, carry bit should be set */
	m->pc = 0;
	m->sfr[SFR_ACC] = 0x10;
	m->sfr[SFR_PSW] = 0x00;
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, 0x11, 0x23), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->sfr[SFR_PSW], PSW_C); /* carry is set */
	assert_int_equal(m->pc, 0x23); /* should branch */
	assert_emu51_callbacks(data, CB_SFR_UPDATE); /* carry in PSW is updated */
	/* A > #data */
	m->pc = 0;
	m->sfr[SFR_ACC] = 0x10;
	m->sfr[SFR_PSW] = 0xff;
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, 0x09, 0x23), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->sfr[SFR_PSW], 0xff ^ PSW_C); /* carry is cleared */
	assert_int_equal(m->pc, 0x23); /* should branch */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* A == #data */
	m->pc = 0;
	m->sfr[SFR_ACC] = 0x10;
	m->sfr[SFR_PSW] = 0xff;
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, 0x10, 0x23), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->sfr[SFR_PSW], 0xff ^ PSW_C); /* carry is cleared */
	assert_int_equal(m->pc, 0x0); /* should not branch when equal */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);

	/* CJNE A, iram addr, reladdr (opcode=0xb5) */
	/* A < (addr), addr < 128 */
	opcode = 0xb5;
	addr = 0x54;
	m->pc = 0;
	m->sfr[SFR_ACC] = 33;
	m->iram_lower[addr] = 34;
	m->sfr[SFR_PSW] = 0;
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, addr, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 0x30); /* should branch */
	assert_int_equal(m->sfr[SFR_PSW], PSW_C); /* carry is set */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* A == (addr), addr < 128 */
	opcode = 0xb5;
	addr = 0x54;
	m->pc = 0;
	m->sfr[SFR_ACC] = 34;
	m->iram_lower[addr] = 34;
	m->sfr[SFR_PSW] = 0xff;
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, addr, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 0); /* should not branch */
	assert_int_equal(m->sfr[SFR_PSW], 0xff ^ PSW_C); /* carry is clear */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* A > (addr), addr < 128 */
	opcode = 0xb5;
	addr = 0x54;
	m->pc = 0;
	m->sfr[SFR_ACC] = 35;
	m->iram_lower[addr] = 34;
	m->sfr[SFR_PSW] = 0xff;
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, addr, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 0x30); /* should branch */
	assert_int_equal(m->sfr[SFR_PSW], 0xff ^ PSW_C); /* carry is clear */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* A < (addr), addr >= 128 (SFR) */
	opcode = 0xb5;
	m->pc = 0;
	addr = SFR_BASE_ADDR + SFR_B; /* register B */
	m->sfr[SFR_B] = 34;
	m->sfr[SFR_ACC] = 33;
	m->sfr[SFR_PSW] = 0;
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, addr, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 0x30); /* should branch */
	assert_int_equal(m->sfr[SFR_PSW], PSW_C); /* carry is set */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* A == (addr), addr >= 128 (SFR) */
	opcode = 0xb5;
	m->pc = 0;
	addr = SFR_BASE_ADDR + SFR_B; /* register B */
	m->sfr[SFR_B] = 34;
	m->sfr[SFR_ACC] = 34;
	m->sfr[SFR_PSW] = 0xff;
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, addr, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 0); /* should not branch */
	assert_int_equal(m->sfr[SFR_PSW], 0xff ^ PSW_C); /* carry is clear */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* A > (addr), addr >= 128 (SFR) */
	opcode = 0xb5;
	m->pc = 0;
	addr = SFR_BASE_ADDR + SFR_B; /* register B */
	m->sfr[SFR_B] = 34;
	m->sfr[SFR_ACC] = 35;
	m->sfr[SFR_PSW] = 0xff;
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, addr, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 0x30); /* should branch */
	assert_int_equal(m->sfr[SFR_PSW], 0xff ^ PSW_C); /* carry is clear */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);

	/* CJNE @R0, #data, reladdr (opcode = 0xb6) */
	opcode = 0xb6;
	/* (R0) < data */
	m->pc = 3;
	addr = 0x85;
	R0(m) = addr;
	PSW(m) = 0;
	iram_write(m, addr, 0x44);
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, 0x45, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 0x30 + 3); /* should branch */
	assert_int_equal(PSW(m), PSW_C); /* carry is set */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* (R0) == data */
	m->pc = 3;
	addr = 0x85;
	R0(m) = addr;
	PSW(m) = PSW_C;
	iram_write(m, addr, 0x45);
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, 0x45, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 3); /* should not branch */
	assert_int_equal(PSW(m) & PSW_C, 0); /* carry is clear */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* (R0) > data */
	m->pc = 3;
	addr = 0x85;
	R0(m) = addr;
	PSW(m) = PSW_C;
	iram_write(m, addr, 0x46);
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, 0x45, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 0x30 + 3); /* should branch */
	assert_int_equal(PSW(m) & PSW_C, 0); /* carry is clear */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* index out of range */
	testdata *data_no_upper_iram = dup_test_data(data);
	data_no_upper_iram->m->iram_upper = NULL;
	m->pc = 3;
	addr = 0x85; /* this address belongs to the upper iram */
	R0(m) = addr;
	PSW(m) = PSW_C;
	iram_write(m, addr, 0x46);
	err = run_instr(INSTR3(opcode, 0x45, 0x30), data_no_upper_iram);
	assert_int_equal(err, EMU51_IRAM_OUT_OF_RANGE);
	assert_int_equal(m->pc, 3); /* pc should stay the same */
	assert_int_equal(PSW(m) & PSW_C, PSW_C); /* carry is not changed */
	assert_emu51_callbacks(data_no_upper_iram, 0); /* no callback is called */
	free_test_data(data_no_upper_iram);

	/* CJNE @R1, #data, reladdr (opcode = 0xb7) */
	opcode = 0xb7;
	/* (R1) < data */
	m->pc = 3;
	addr = 0x87;
	PSW(m) = 0;
	R1(m) = addr;
	iram_write(m, addr, 0x44);
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, 0x45, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 0x30 + 3); /* should branch */
	assert_int_equal(PSW(m), PSW_C); /* carry is set */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* (R1) == data */
	m->pc = 3;
	addr = 0x87;
	R1(m) = addr;
	PSW(m) = PSW_C;
	iram_write(m, addr, 0x45);
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, 0x45, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 3); /* should not branch */
	assert_int_equal(PSW(m) & PSW_C, 0); /* carry is clear */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);
	/* (R1) > data */
	m->pc = 3;
	addr = 0x87;
	R1(m) = addr;
	PSW(m) = PSW_C;
	iram_write(m, addr, 0x46);
	expect_value(callback_sfr_update, index, SFR_PSW);
	err = run_instr(INSTR3(opcode, 0x45, 0x30), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 0x30 + 3); /* should branch */
	assert_int_equal(PSW(m) & PSW_C, 0); /* carry is clear */
	assert_emu51_callbacks(data, CB_SFR_UPDATE);

	/* CJNE Rn, #data, reladdr (opcode: 0xb8~0xbf for R0~R7) */
	int i;
	for (i = 0; i <= 7; i++) {
		opcode = 0xb8 + i;
		/* Rn < data */
		m->pc = 5;
		R_REG(m, i) = 0x6f;
		PSW(m) = 0;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR3(opcode, 0x70, 0x32), data);
		assert_int_equal(err, 0);
		assert_int_equal(m->pc, 5 + 0x32); /* should branch */
		assert_int_equal(PSW(m) & PSW_C, PSW_C); /* carry set */
		assert_emu51_callbacks(data, CB_SFR_UPDATE);
		/* Rn == data */
		m->pc = 5;
		R_REG(m, i) = 0x70;
		PSW(m) = PSW_C;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR3(opcode, 0x70, 0x32), data);
		assert_int_equal(err, 0);
		assert_int_equal(m->pc, 5); /* should not branch */
		assert_int_equal(PSW(m) & PSW_C, 0); /* carry clear */
		assert_emu51_callbacks(data, CB_SFR_UPDATE);
		/* Rn > data */
		m->pc = 5;
		R_REG(m, i) = 0x71;
		PSW(m) = PSW_C;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR3(opcode, 0x70, 0x32), data);
		assert_int_equal(err, 0);
		assert_int_equal(m->pc, 5 + 0x32); /* should branch */
		assert_int_equal(PSW(m) & PSW_C, 0); /* carry clear */
		assert_emu51_callbacks(data, CB_SFR_UPDATE);
	}

	free_test_data(data);
}

/* end of test functions */

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_nop),
		cmocka_unit_test(test_ajmp),
		cmocka_unit_test(test_jmp),
		cmocka_unit_test(test_ljmp),
		cmocka_unit_test(test_sjmp),
		cmocka_unit_test(test_movc),
		cmocka_unit_test(test_cjne),
	};
	/* don't use setup and teardown as cmocka doesn't report memory bugs in them
	 */
	return cmocka_run_group_tests(tests, NULL, NULL);
}
