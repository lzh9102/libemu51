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
} testdata;

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

/* the highest byte (0xff000000) is used to store instruction length */
#define INSTR1(opcode) ((0x01 << 24) | (opcode))
#define INSTR2(opcode, op1) ((0x02 << 24) | ((op1) << 8) | (opcode))
#define INSTR3(opcode, op1, op2) ((0x03 << 24) | \
		((op2) << 16) | ((op1) << 8) | (opcode))

/* put the instruction in pc and run it */
int run_instr(uint32_t instr, testdata *data)
{
	uint8_t instr_length = (instr >> 24) & 0xff;
	uint8_t buffer[3];

	buffer[0] = instr & 0xff;
	buffer[1] = (instr >> 8) & 0xff;
	buffer[2] = (instr >> 16) & 0xff;

	uint8_t opcode = buffer[0];

	const emu51_instr *instr_info = _emu51_decode_instr(opcode);
	assert_int_equal(instr_info->bytes, instr_length);
	assert_non_null(instr_info->handler);

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
	assert_int_equal(data->m->pc, target_); } while (0)

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
	data->pmem[0] = opcode;
	data->pmem[1] = UPPER_BYTE(target_addr);
	data->pmem[2] = LOWER_BYTE(target_addr);
	const emu51_instr *instr = _emu51_decode_instr(opcode);
	instr->handler(instr, data->pmem, data->m);

	/* ljmp shouldn't change any of the memories */
	assert_emu51_all_ram_equal(data, orig_data);

	/* ljmp should set the pc to the target address */
	assert_int_equal(data->m->pc, target_addr);

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
	data->pmem[0] = opcode;
	data->pmem[1] = reladdr;
	m->pc = 0;
	const emu51_instr *instr = _emu51_decode_instr(opcode);
	int err = instr->handler(instr, data->pmem, data->m);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 127);
	assert_emu51_all_ram_equal(data, orig_data);

	/* sjmp backwards */
	opcode = 0x80;
	reladdr = 0x80; /* signed -128 */
	data->pmem[254] = opcode;
	data->pmem[255] = reladdr;
	m->pc = 254;
	instr = _emu51_decode_instr(opcode);
	err = instr->handler(instr, &data->pmem[254], data->m);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 254 - 128);

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
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->sfr[SFR_ACC], 2);

	/* test boundary condition */
	SET_DPTR(m, PMEM_SIZE-1); /* in bounds */
	m->sfr[SFR_ACC] = 0;
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, 0);
	SET_DPTR(m, PMEM_SIZE); /* out of bounds */
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, EMU51_PMEM_OUT_OF_RANGE);

	SET_DPTR(m, 0);

	/* MOVC A, @A+PC */
	opcode = 0x83;
	m->pc = 140;
	m->sfr[SFR_ACC] = 7;
	data->pmem[140] = 1; /* this is not the data to read */
	data->pmem[147] = 2; /* this is the data to read (DPTR+ACC) */
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->sfr[SFR_ACC], 2);

	/* test boundary condition */
	m->pc = PMEM_SIZE - 1; /* in bounds */
	m->sfr[SFR_ACC] = 0;
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, 0);
	m->pc = PMEM_SIZE; /* out of bounds */
	err = run_instr(INSTR1(opcode), data);
	assert_int_equal(err, EMU51_PMEM_OUT_OF_RANGE);

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
	};
	/* don't use setup and teardown as cmocka doesn't report memory bugs in them
	 */
	return cmocka_run_group_tests(tests, NULL, NULL);
}
