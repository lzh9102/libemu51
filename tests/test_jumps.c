/* tests for jump/call instructions */

#include "test_instr_common.h"

/* disable unused parameter warning when using gcc */
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

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

void test_acall(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;
	int err;
	uint8_t opcode;

	/* ACALL page0 */
	opcode = 0x11;
	PC(m) = 0xffaa;
	SP(m) = 0x20;
	expect_value(callback_sfr_update, index, SFR_SP);
	expect_value(callback_iram_update, addr, 0x21);
	expect_value(callback_iram_update, addr, 0x22);
	err = run_instr(INSTR2(opcode, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xf810);
	assert_int_equal(SP(m), 0x22);
	assert_int_equal(iram_read(m, 0x21), 0xaa);
	assert_int_equal(iram_read(m, 0x22), 0xff);
	assert_emu51_callbacks(data, CB_SFR_UPDATE | CB_IRAM_UPDATE);

	/* ACALL page1 */
	opcode = 0x31;
	PC(m) = 0xffaa;
	SP(m) = 0x20;
	expect_value(callback_sfr_update, index, SFR_SP);
	expect_value(callback_iram_update, addr, 0x21);
	expect_value(callback_iram_update, addr, 0x22);
	err = run_instr(INSTR2(opcode, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xf910);
	assert_int_equal(SP(m), 0x22);
	assert_int_equal(iram_read(m, 0x21), 0xaa);
	assert_int_equal(iram_read(m, 0x22), 0xff);
	assert_emu51_callbacks(data, CB_SFR_UPDATE | CB_IRAM_UPDATE);

	/* ACALL page2 */
	opcode = 0x51;
	PC(m) = 0xffaa;
	SP(m) = 0x20;
	expect_value(callback_sfr_update, index, SFR_SP);
	expect_value(callback_iram_update, addr, 0x21);
	expect_value(callback_iram_update, addr, 0x22);
	err = run_instr(INSTR2(opcode, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xfa10);
	assert_int_equal(SP(m), 0x22);
	assert_int_equal(iram_read(m, 0x21), 0xaa);
	assert_int_equal(iram_read(m, 0x22), 0xff);
	assert_emu51_callbacks(data, CB_SFR_UPDATE | CB_IRAM_UPDATE);

	/* ACALL page3 */
	opcode = 0x71;
	PC(m) = 0xffaa;
	SP(m) = 0x20;
	expect_value(callback_sfr_update, index, SFR_SP);
	expect_value(callback_iram_update, addr, 0x21);
	expect_value(callback_iram_update, addr, 0x22);
	err = run_instr(INSTR2(opcode, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xfb10);
	assert_int_equal(SP(m), 0x22);
	assert_int_equal(iram_read(m, 0x21), 0xaa);
	assert_int_equal(iram_read(m, 0x22), 0xff);
	assert_emu51_callbacks(data, CB_SFR_UPDATE | CB_IRAM_UPDATE);

	/* ACALL page4 */
	opcode = 0x91;
	PC(m) = 0xffaa;
	SP(m) = 0x20;
	expect_value(callback_sfr_update, index, SFR_SP);
	expect_value(callback_iram_update, addr, 0x21);
	expect_value(callback_iram_update, addr, 0x22);
	err = run_instr(INSTR2(opcode, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xfc10);
	assert_int_equal(SP(m), 0x22);
	assert_int_equal(iram_read(m, 0x21), 0xaa);
	assert_int_equal(iram_read(m, 0x22), 0xff);
	assert_emu51_callbacks(data, CB_SFR_UPDATE | CB_IRAM_UPDATE);

	/* ACALL page5 */
	opcode = 0xb1;
	PC(m) = 0xffaa;
	SP(m) = 0x20;
	expect_value(callback_sfr_update, index, SFR_SP);
	expect_value(callback_iram_update, addr, 0x21);
	expect_value(callback_iram_update, addr, 0x22);
	err = run_instr(INSTR2(opcode, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xfd10);
	assert_int_equal(SP(m), 0x22);
	assert_int_equal(iram_read(m, 0x21), 0xaa);
	assert_int_equal(iram_read(m, 0x22), 0xff);
	assert_emu51_callbacks(data, CB_SFR_UPDATE | CB_IRAM_UPDATE);

	/* ACALL page6 */
	opcode = 0xd1;
	PC(m) = 0x00aa;
	SP(m) = 0x20;
	expect_value(callback_sfr_update, index, SFR_SP);
	expect_value(callback_iram_update, addr, 0x21);
	expect_value(callback_iram_update, addr, 0x22);
	err = run_instr(INSTR2(opcode, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x0610);
	assert_int_equal(SP(m), 0x22);
	assert_int_equal(iram_read(m, 0x21), 0xaa);
	assert_int_equal(iram_read(m, 0x22), 0x00);
	assert_emu51_callbacks(data, CB_SFR_UPDATE | CB_IRAM_UPDATE);

	/* ACALL page7 */
	opcode = 0xf1;
	PC(m) = 0x00aa;
	SP(m) = 0x20;
	expect_value(callback_sfr_update, index, SFR_SP);
	expect_value(callback_iram_update, addr, 0x21);
	expect_value(callback_iram_update, addr, 0x22);
	err = run_instr(INSTR2(opcode, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x0710);
	assert_int_equal(SP(m), 0x22);
	assert_int_equal(iram_read(m, 0x21), 0xaa);
	assert_int_equal(iram_read(m, 0x22), 0x00);
	assert_emu51_callbacks(data, CB_SFR_UPDATE | CB_IRAM_UPDATE);

	/* TODO: test for stack overflow (e.g. no upper ram */

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

void test_jc_jnc(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;
	int err;

	/* JC: jump forward, carry set */
	PC(m) = 0xaaaa;
	PSW(m) |= PSW_C;
	err = run_instr(INSTR2(0x40, 0x7f), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xaaaa + 0x7f);
	/* JNC: don't jump */
	PC(m) = 0xaaaa;
	PSW(m) |= PSW_C;
	err = run_instr(INSTR2(0x50, 0x7f), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xaaaa);

	/* JC: not jumping forward */
	PC(m) = 0xaaaa;
	PSW(m) &= ~PSW_C; /* carry not set */
	err = run_instr(INSTR2(0x40, 0x7f), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xaaaa);
	/* JNC: jump forward */
	PC(m) = 0xaaaa;
	PSW(m) &= ~PSW_C; /* carry not set */
	err = run_instr(INSTR2(0x50, 0x7f), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xaaaa + 0x7f);

	/* JC: jump backward, carry set */
	PC(m) = 0xaaaa;
	PSW(m) |= PSW_C;
	err = run_instr(INSTR2(0x40, 0x80), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xaaaa - 0x80);
	/* JNC: don't jump */
	PC(m) = 0xaaaa;
	PSW(m) |= PSW_C;
	err = run_instr(INSTR2(0x50, 0x80), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xaaaa);

	/* JC: not jumping backward */
	PC(m) = 0xaaaa;
	PSW(m) &= ~PSW_C; /* carry not set */
	err = run_instr(INSTR2(0x40, 0x80), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xaaaa);
	/* JNC: jump backward */
	PC(m) = 0xaaaa;
	PSW(m) &= ~PSW_C; /* carry not set */
	err = run_instr(INSTR2(0x50, 0x80), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0xaaaa - 0x80);

	free_test_data(data);
}

void test_jz_jnz(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;
	int err;

	/* JZ: don't jump when A=0xff */
	PC(m) = 0x30;
	ACC(m) = 0xff;
	err = run_instr(INSTR2(0x60, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x30);
	/* JNZ: jump */
	PC(m) = 0x30;
	ACC(m) = 0xff;
	err = run_instr(INSTR2(0x70, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x40);

	/* JZ: jump when A=0, forward */
	PC(m) = 0x30;
	ACC(m) = 0;
	err = run_instr(INSTR2(0x60, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x40);
	/* JNZ: don't jump */
	PC(m) = 0x30;
	ACC(m) = 0;
	err = run_instr(INSTR2(0x70, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x30);

	/* JZ: jump when A=0, backward */
	PC(m) = 0x30;
	ACC(m) = 0;
	err = run_instr(INSTR2(0x60, 0xf0), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x20);

	/* JZ: don't jump when A=1 */
	PC(m) = 0x30;
	ACC(m) = 1;
	err = run_instr(INSTR2(0x60, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x30);
	/* JNZ: jump */
	PC(m) = 0x30;
	ACC(m) = 1;
	err = run_instr(INSTR2(0x70, 0x10), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x40);
	/* JNZ: jump backwards */
	PC(m) = 0x30;
	ACC(m) = 1;
	err = run_instr(INSTR2(0x70, 0xf0), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x20);

	free_test_data(data);
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

void test_lcall(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;
	int err;

	PC(m) = 0x1234;
	SP(m) = 0x30;
	iram_write(m, 0x30, 0xaa);
	iram_write(m, 0x31, 0xff);
	iram_write(m, 0x32, 0xff);
	expect_value(callback_sfr_update, index, SFR_SP);
	expect_value(callback_iram_update, addr, 0x31);
	expect_value(callback_iram_update, addr, 0x32);
	err = run_instr(INSTR3(0x12, 0x57, 0x83), data);
	assert_int_equal(err, 0);
	assert_int_equal(PC(m), 0x5783);
	assert_int_equal(SP(m), 0x32);
	assert_int_equal(iram_read(m, 0x30), 0xaa);
	assert_int_equal(iram_read(m, 0x31), 0x34);
	assert_int_equal(iram_read(m, 0x32), 0x12);
	assert_emu51_callbacks(data, CB_SFR_UPDATE | CB_IRAM_UPDATE);

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
		m->pc = 30;
		R_REG(m, i) = 0x6f;
		PSW(m) = 0;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR3(opcode, 0x70, -5), data);
		assert_int_equal(err, 0);
		assert_int_equal(m->pc, 25); /* should branch */
		assert_int_equal(PSW(m) & PSW_C, PSW_C); /* carry set */
		assert_emu51_callbacks(data, CB_SFR_UPDATE);
		/* Rn == data */
		m->pc = 30;
		R_REG(m, i) = 0x70;
		PSW(m) = PSW_C;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR3(opcode, 0x70, -5), data);
		assert_int_equal(err, 0);
		assert_int_equal(m->pc, 30); /* should not branch */
		assert_int_equal(PSW(m) & PSW_C, 0); /* carry clear */
		assert_emu51_callbacks(data, CB_SFR_UPDATE);
		/* Rn > data */
		m->pc = 30;
		R_REG(m, i) = 0x71;
		PSW(m) = PSW_C;
		expect_value(callback_sfr_update, index, SFR_PSW);
		err = run_instr(INSTR3(opcode, 0x70, -5), data);
		assert_int_equal(err, 0);
		assert_int_equal(m->pc, 25); /* should branch */
		assert_int_equal(PSW(m) & PSW_C, 0); /* carry clear */
		assert_emu51_callbacks(data, CB_SFR_UPDATE);
	}

	free_test_data(data);
}

void test_djnz(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;
	uint8_t iram_addr;
	int8_t reladdr;
	int err;
	int orig_pc = 70;
	int regno;

	/* DJNZ iram addr, reladdr (opcode = 0xd5) */

	iram_addr = SFR_ACC + SFR_BASE_ADDR; /* ACC */
	m->sfr[SFR_ACC] = 2; /* initial A = 2 */
	reladdr = -63;
	m->pc = orig_pc;

	/* djnz A, 63 (before: A = 2; after: A = 1; jump: true) */
	expect_value(callback_sfr_update, index, SFR_ACC);
	err = run_instr(INSTR3(0xd5, iram_addr, reladdr), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, orig_pc + reladdr); /* jumps */
	assert_int_equal(m->sfr[SFR_ACC], 1);
	assert_emu51_callbacks(data, CB_SFR_UPDATE);

	/* djnz A, 63 (before: A = 1; after: A = 0; jump: false) */
	m->pc = orig_pc;
	expect_value(callback_sfr_update, index, SFR_ACC);
	err = run_instr(INSTR3(0xd5, iram_addr, reladdr), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, orig_pc); /* does not jump */
	assert_int_equal(m->sfr[SFR_ACC], 0);
	assert_emu51_callbacks(data, CB_SFR_UPDATE);

	/* djnz A, 63 (before: A = 0; after: A = 255; jump: true) */
	expect_value(callback_sfr_update, index, SFR_ACC);
	m->pc = orig_pc;
	err = run_instr(INSTR3(0xd5, iram_addr, reladdr), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, orig_pc + reladdr); /* jumps */
	assert_int_equal(m->sfr[SFR_ACC], 255);
	assert_emu51_callbacks(data, CB_SFR_UPDATE);

	/* DJNZ Rn, reladdr (opcode = 0xd8~0xdf) */
	for (regno = 0; regno < 8; regno++) {
		int opcode = 0xd8 + regno;

		R_REG(m, regno) = 2;

		/* djnz Rn, -32 (before: Rn = 2; after: Rn = 1; jump: true) */
		m->pc = 64;
		err = run_instr(INSTR2(opcode, -32), data);
		assert_int_equal(err, 0);
		assert_int_equal(m->pc, 64 - 32);
		assert_int_equal(R_REG(m, regno), 1);

		/* djnz Rn, -32 (before: Rn = 1: after: Rn = 0: jump: false) */
		m->pc = 64;
		err = run_instr(INSTR2(opcode, -32), data);
		assert_int_equal(err, 0);
		assert_int_equal(m->pc, 64);
		assert_int_equal(R_REG(m, regno), 0);

		/* djnz Rn, -32 (before: Rn = 0: after: Rn = 255: jump: true) */
		m->pc = 64;
		err = run_instr(INSTR2(opcode, -32), data);
		assert_int_equal(err, 0);
		assert_int_equal(m->pc, 64 - 32);
		assert_int_equal(R_REG(m, regno), 255);
	}

	free_test_data(data);
}

void test_jb(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;
	uint8_t opcode = 0x20;
	int err;

	m->pc = 128;
	m->iram_lower[0x20] = 0x08; /* bit[3] */
	err = run_instr(INSTR3(opcode, 3, -8), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 120); /* jumps */
	assert_int_equal(m->iram_lower[0x20], 0x08); /* jb doesn't change the bit */

	m->pc = 128;
	m->iram_lower[0x20] = 0x00;
	err = run_instr(INSTR3(opcode, 3, -8), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 128); /* doesn't jump */
	assert_int_equal(m->iram_lower[0x20], 0x00); /* jb doesn't change the bit */

	/* bit address >= 128 is invalid */
	err = run_instr(INSTR3(opcode, 128, 0), data);
	assert_int_equal(err, EMU51_BIT_OUT_OF_RANGE);

	free_test_data(data);
}

void test_jbc(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;
	uint8_t opcode = 0x10;
	int err;

	m->pc = 128;
	m->iram_lower[0x20] = 0x08; /* bit[3] */
	err = run_instr(INSTR3(opcode, 3, -8), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 120); /* jumps */
	assert_int_equal(m->iram_lower[0x20], 0x00); /* jbc clears the bit after jump */

	m->pc = 128;
	m->iram_lower[0x20] = 0x00;
	err = run_instr(INSTR3(opcode, 3, -8), data);
	assert_int_equal(err, 0);
	assert_int_equal(m->pc, 128); /* doesn't jump */
	assert_int_equal(m->iram_lower[0x20], 0x00); /* jb doesn't change the bit */

	/* bit address >= 128 is invalid */
	err = run_instr(INSTR3(opcode, 128, 0), data);
	assert_int_equal(err, EMU51_BIT_OUT_OF_RANGE);

	free_test_data(data);
}

/* end of test functions */

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_nop),
		cmocka_unit_test(test_acall),
		cmocka_unit_test(test_ajmp),
		cmocka_unit_test(test_jmp),
		cmocka_unit_test(test_jc_jnc),
		cmocka_unit_test(test_jz_jnz),
		cmocka_unit_test(test_ljmp),
		cmocka_unit_test(test_lcall),
		cmocka_unit_test(test_sjmp),
		cmocka_unit_test(test_movc),
		cmocka_unit_test(test_cjne),
		cmocka_unit_test(test_djnz),
		cmocka_unit_test(test_jb),
		cmocka_unit_test(test_jbc),
	};
	/* don't use setup and teardown as cmocka doesn't report memory bugs in them
	 */
	return cmocka_run_group_tests(tests, NULL, NULL);
}
