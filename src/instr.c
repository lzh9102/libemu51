#include <emu51.h>
#include "instr.h"
#include "helpers.h"

/* Implementations of 8051/8052 instructions.
 *
 * The functions should return 0 on success or an error code on error.
 */

/* disable unused parameter warning when using gcc */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/* These macros are used for writing instruction handlers. By using these
 * macros, it is possible to change the handler signature (e.g. for performance
 * reasons) without replacing each handler function.
 *
 * The emulator object should always be called m in the argument list.
 */
#define DEFINE_HANDLER(name) static int name ( \
		const emu51_instr *instr, const uint8_t *code, emu51 *m)
#define OPCODE code[0]
#define OPERAND1 code[1]
#define OPERAND2 code[2]
#define PC m->pc

#define DPTR ((m->sfr[SFR_DPH] << 8) | m->sfr[SFR_DPL])
#define ACC m->sfr[SFR_ACC]
#define PSW m->sfr[SFR_PSW]
#define SP m->sfr[SFR_SP]

#define BANK_BASE_ADDR (m->sfr[SFR_PSW] & (PSW_RS1 | PSW_RS0))
#define REG_R(n) m->iram_lower[BANK_BASE_ADDR + (n)]

/* Call the callback if it is not NULL. */
#define CALLBACK(cb_name, ...) do { \
	if (m->callback.cb_name) m->callback.cb_name(m, __VA_ARGS__); } while (0)

/* operation: NOP
 * function: consume 1 cycle and do nothing
 */
DEFINE_HANDLER(nop_handler)
{
	return 0;
}

/* operation: ACALL
 * function: absolute call within 2k block
 */
DEFINE_HANDLER(acall_handler)
{
	/* push PC onto the stack, least-significant-byte first,
	 * most-significant-byte second. */
	int err = stack_push(m, PC & 0xff);
	if (err)
		return err;
	err = stack_push(m, (PC >> 8) & 0xff);
	if (err)
		return err;

	/* the first 3 bit of the opcode is the page number */
	int page = (OPCODE >> 5) & 0x7;

	/* replace the lower 11 bits of PC with {page, OPERAND1} */
	PC = (PC & 0xf800) | (page << 8) | OPERAND1;

	/* callbacks */
	CALLBACK(sfr_update, SFR_SP);
	CALLBACK(iram_update, SP - 1);
	CALLBACK(iram_update, SP);

	return 0;
}

/* operation: AJMP
 * function: absolute jump within 2k block
 */
DEFINE_HANDLER(ajmp_handler)
{
	/* the first 3 bit of the opcode is the page number */
	int page = (OPCODE >> 5) & 0x7;

	/* replace the lower 11 bits of PC with {page, OPERAND1} */
	PC &= 0xf800; /* clear the lower 11 bits */
	PC |= (page << 8) | OPERAND1; /* set the lower 11 bits to target */

	return 0;
}

/* operation: JMP
 * function: jump to the sum of DPTR and ACC
 */
DEFINE_HANDLER(jmp_handler)
{
	PC = DPTR + ACC;
	return 0;
}

/* operation: JC
 * function: jump if carry set
 */
DEFINE_HANDLER(jc_handler)
{
	int8_t reladdr = OPERAND1; /* reladdr is signed -128~127 */

	if ((PSW & PSW_C) == PSW_C)
		relative_jump(m, reladdr);
	return 0;
}

/* operation: JNC
 * function: jump if carry not set
 */
DEFINE_HANDLER(jnc_handler)
{
	int8_t reladdr = OPERAND1; /* reladdr is signed -128~127 */

	if ((PSW & PSW_C) == 0)
		relative_jump(m, reladdr);
	return 0;
}


/* operation: JZ
 * function: jump if accumulator zero
 */
DEFINE_HANDLER(jz_handler)
{
	int8_t reladdr = OPERAND1; /* -128~127 */

	if (ACC == 0)
		relative_jump(m, reladdr);
	return 0;
}

/* operation: JNZ
 * function: jump if accumulator not zero
 */
DEFINE_HANDLER(jnz_handler)
{
	int8_t reladdr = OPERAND1; /* -128~127 */

	if (ACC != 0)
		relative_jump(m, reladdr);
	return 0;
}

/* operation: LJMP
 * function: long jump
 */
DEFINE_HANDLER(ljmp_handler)
{
	uint16_t target_addr = (OPERAND1 << 8) | OPERAND2;
	PC = target_addr;
	return 0;
}

/* operation: LCALL
 * function: long call
 */
DEFINE_HANDLER(lcall_handler)
{
	/* push PC onto stack, low-byte first, high-byte second. */
	int err = stack_push(m, PC & 0xff);
	if (err)
		return err;
	err = stack_push(m, (PC >> 8) & 0xff);
	if (err)
		return err;

	/* set PC to target address */
	uint16_t target_addr = (OPERAND1 << 8) | OPERAND2;
	PC = target_addr;

	/* callbacks */
	CALLBACK(sfr_update, SFR_SP);
	CALLBACK(iram_update, SP - 1);
	CALLBACK(iram_update, SP);

	return 0;
}

/* operation: SJMP
 * function: relative jump -128~+127 bytes
 */
DEFINE_HANDLER(sjmp_handler)
{
	int8_t reladdr = OPERAND1;

	relative_jump(m, reladdr);

	return 0;
}

/* operation: MOVC A, @A+DPTR
 */
DEFINE_HANDLER(movc_dptr_handler)
{
	uint16_t addr = ACC + DPTR;

	/* check if the target address is outside valid program memory */
	if (addr >= m->pmem_len)
		return EMU51_PMEM_OUT_OF_RANGE;

	ACC = m->pmem[addr];
	CALLBACK(sfr_update, SFR_ACC);

	return 0;
}

/* operation: MOVC A, @A+PC
 */
DEFINE_HANDLER(movc_pc_handler)
{
	uint16_t addr = ACC + PC;

	/* check if the target address is outside valid program memory */
	if (addr >= m->pmem_len)
		return EMU51_PMEM_OUT_OF_RANGE;

	ACC = m->pmem[addr];
	CALLBACK(sfr_update, SFR_ACC);

	return 0;
}

/* perform CJNE given value of operand1, operand2 and reladdr */
static inline int general_cjne(emu51 *m, uint8_t op1, uint8_t op2,
		int8_t reladdr)
{
	/* compute carry flag: set only when op1 < op2 */
	if (op1 < op2)
		PSW |= PSW_C;
	else
		PSW &= ~PSW_C;

	/* branch if not equal */
	if (op1 != op2)
		relative_jump(m, reladdr);

	/* PSW is updated */
	CALLBACK(sfr_update, SFR_PSW);

	return 0;
}

/* operation: CJNE A, #data, reladdr
 * flags: carry
 */
DEFINE_HANDLER(cjne_a_data_handler)
{
	uint8_t data = OPERAND1;
	int8_t reladdr = OPERAND2;

	return general_cjne(m, ACC, data, reladdr);
}

/* operation: CJNE A, iram addr, reladdr
 * flags: carry
 */
DEFINE_HANDLER(cjne_a_addr_handler)
{
	uint8_t addr = OPERAND1;
	int8_t reladdr = OPERAND2;
	uint8_t data = direct_addr_read(m, addr);

	return general_cjne(m, ACC, data, reladdr);
}

/* operation: CJNE @R0, #data, reladdr
 * flags: carry
 */
DEFINE_HANDLER(cjne_deref_r_data_handler)
{
	uint8_t data = OPERAND1;
	int8_t reladdr = OPERAND2;

	/* The last bit of opcode determines whether to use R0 or R1.
	 * last bit == 0 => R0
	 * last bit == 1 => R1
	 */
	uint8_t addr = BANK_BASE_ADDR + (OPCODE & 0x01);

	/* get the value of @R0 or R1 */
	uint8_t reg_derefenced_value;
	int err = indirect_addr_read(m, addr, &reg_derefenced_value);
	if (err)
		return err;

	return general_cjne(m, reg_derefenced_value, data, reladdr);
}

/* operation: CJNE Rn, #data, reladdr
 * flags: carry
 */
DEFINE_HANDLER(cjne_r_data_handler)
{
	uint8_t data = OPERAND1;
	int8_t reladdr = OPERAND2;

	/* The last 3 bit of opcode is the R number (0~7) */
	uint8_t r_index = OPCODE & 0x07;

	return general_cjne(m, REG_R(r_index), data, reladdr);
}

/* operation: DJNZ iram addr, reladdr
 */
DEFINE_HANDLER(djnz_iram_handler)
{
	uint8_t iram_addr = OPERAND1;
	int8_t reladdr = OPERAND2;

	/* decrement the data */
	uint8_t new_value = direct_addr_read(m, iram_addr) - 1;
	direct_addr_write(m, iram_addr, new_value);

	/* jump if data is not zero after decrementing */
	if (new_value != 0)
		relative_jump(m, reladdr);

	/* ACC is updated */
	CALLBACK(sfr_update, SFR_ACC);

	return 0;
}

/* operation: JB  bit addr, reladdr (opcode = 0x20)
 *            JBC bit addr, reladdr (opcode = 0x10)
 *            JNB bit addr, reladdr (opcode = 0x30)
 */
DEFINE_HANDLER(jump_if_bit_handler)
{
	uint8_t bit_addr = OPERAND1;
	int8_t reladdr = OPERAND2;
	int jump_value = (OPCODE == 0x30) ? 0 : 1;

	int bit_value = bit_read(m, bit_addr);
	if (bit_value < 0) /* error */
		return bit_value;

	if (bit_value == jump_value) {
		/* clear the bit if the instruction is JBC */
		if (OPCODE == 0x10)
			bit_write(m, bit_addr, 0);
		relative_jump(m, reladdr);
	}

	return 0;
}

/* operation: DJNZ Rn, reladdr
 */
DEFINE_HANDLER(djnz_r_handler)
{
	uint8_t regno = OPCODE & 0x07;
	int8_t reladdr = OPERAND1;

	/* decrement Rn and jump if new value is not zero */
	if (--REG_R(regno))
		relative_jump(m, reladdr);

	return 0;
}

/* ACC <- ACC + operand + carry_in
 * affected flags: C, AC, OV
 */
static void general_add(emu51 *m, uint8_t operand, uint8_t carry_in)
{
#define signbit(byte) ((byte) & 0x80)
	uint16_t sum_with_carry;
	carry_in &= 1; /* only the lowest bit of carry_in is used */
	sum_with_carry = (uint16_t)ACC + (uint16_t)operand + carry_in;

	PSW &= ~(PSW_C | PSW_AC | PSW_OV); /* clear flags */

	/* auxiliary carry: carry out at the 4th bit */
	if ((sum_with_carry & 0x0f) != (ACC & 0x0f) + (operand & 0x0f) + carry_in)
		PSW |= PSW_AC;

	/* carry out: sum > 255 */
	if (sum_with_carry > 0xff)
		PSW |= PSW_C;

	/* overflow: adding two numbers of the same sign but get a number with a
	 * different sign
	 */
	if (signbit(ACC) == signbit(operand)
			&& signbit(operand) != signbit(sum_with_carry))
		PSW |= PSW_OV;

	/* write result to ACC */
	ACC = sum_with_carry & 0xff;

	/* callbacks */
	CALLBACK(sfr_update, SFR_PSW);

#undef signbit
}

/* operation: ADD  A, operand (opcode: 0x24~0x2f)
 *            ADDC A, operand (opcode: 0x34~0x3f)
 */
DEFINE_HANDLER(add_handler)
{
	uint8_t carry_in = 0;
	int err = 0;

	/* load addend */
	uint8_t operand;
	switch (OPCODE & 0x0f) {
		case 0x04: /* ADD A, #data */
			operand = OPERAND1;
			break;
		case 0x05: /* ADD A, iram addr */
			operand = direct_addr_read(m, OPERAND1);
			break;
		case 0x06: /* ADD A, @R0 */
		case 0x07: /* ADD A, @R1 */
			err = indirect_addr_read(m, BANK_BASE_ADDR + (OPCODE & 1), &operand);
			if (err)
				return err;
			break;
		default: /* ADD A, Rn */
			operand = REG_R(OPCODE & 0x07);
	}

	/* 0x2* -> ADD (carry_in = 0)
	 * 0x3* -> ADDC (carry_in = carry flag)
	 */
	if ((OPCODE & 0xf0) == 0x30 && (PSW & PSW_C))
		carry_in = 1;

	/* ADD A, #data */
	general_add(m, operand, carry_in);
	return 0;
}

/* End of instruction handlers */

/* restore diagnostic settings */
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

/* macro to define an instruction */
#define INSTR(op, mne, b, c, h) {.opcode = op, .bytes = b, .cycles = c, \
	.handler = h}

/* fill in this macro in the table if the opcode is not implemented */
#define NOT_IMPLEMENTED(op) {.opcode = (op), \
	.bytes = 0, .cycles = 0, .handler = 0}

/* the instruction lookup table: valid range of opcode is 0~255 */
const emu51_instr _emu51_instr_table[256] = {
	/* opcode, mnemonics, bytes, cycles, handler */
	INSTR(0x00, "NOP",  1, 1, nop_handler),
	INSTR(0x01, "AJMP", 2, 2, ajmp_handler),
	INSTR(0x02, "LJMP", 3, 2, ljmp_handler),
	NOT_IMPLEMENTED(0x03),
	NOT_IMPLEMENTED(0x04),
	NOT_IMPLEMENTED(0x05),
	NOT_IMPLEMENTED(0x06),
	NOT_IMPLEMENTED(0x07),
	NOT_IMPLEMENTED(0x08),
	NOT_IMPLEMENTED(0x09),
	NOT_IMPLEMENTED(0x0a),
	NOT_IMPLEMENTED(0x0b),
	NOT_IMPLEMENTED(0x0c),
	NOT_IMPLEMENTED(0x0d),
	NOT_IMPLEMENTED(0x0e),
	NOT_IMPLEMENTED(0x0f),
	INSTR(0x10, "JBC", 3, 2, jump_if_bit_handler),
	INSTR(0x11, "ACALL", 2, 2, acall_handler),
	INSTR(0x12, "LCALL", 3, 2, lcall_handler),
	NOT_IMPLEMENTED(0x13),
	NOT_IMPLEMENTED(0x14),
	NOT_IMPLEMENTED(0x15),
	NOT_IMPLEMENTED(0x16),
	NOT_IMPLEMENTED(0x17),
	NOT_IMPLEMENTED(0x18),
	NOT_IMPLEMENTED(0x19),
	NOT_IMPLEMENTED(0x1a),
	NOT_IMPLEMENTED(0x1b),
	NOT_IMPLEMENTED(0x1c),
	NOT_IMPLEMENTED(0x1d),
	NOT_IMPLEMENTED(0x1e),
	NOT_IMPLEMENTED(0x1f),
	INSTR(0x20, "JB", 3, 2, jump_if_bit_handler),
	INSTR(0x21, "AJMP", 2, 2, ajmp_handler),
	NOT_IMPLEMENTED(0x22),
	NOT_IMPLEMENTED(0x23),
	INSTR(0x24, "ADD", 2, 1, add_handler),
	INSTR(0x25, "ADD", 2, 1, add_handler),
	INSTR(0x26, "ADD", 1, 1, add_handler),
	INSTR(0x27, "ADD", 1, 1, add_handler),
	INSTR(0x28, "ADD", 1, 1, add_handler),
	INSTR(0x29, "ADD", 1, 1, add_handler),
	INSTR(0x2a, "ADD", 1, 1, add_handler),
	INSTR(0x2b, "ADD", 1, 1, add_handler),
	INSTR(0x2c, "ADD", 1, 1, add_handler),
	INSTR(0x2d, "ADD", 1, 1, add_handler),
	INSTR(0x2e, "ADD", 1, 1, add_handler),
	INSTR(0x2f, "ADD", 1, 1, add_handler),
	INSTR(0x30, "JNB", 3, 2, jump_if_bit_handler),
	INSTR(0x31, "ACALL", 2, 2, acall_handler),
	NOT_IMPLEMENTED(0x32),
	NOT_IMPLEMENTED(0x33),
	INSTR(0x34, "ADDC", 2, 1, add_handler),
	INSTR(0x35, "ADDC", 2, 1, add_handler),
	INSTR(0x36, "ADDC", 1, 1, add_handler),
	INSTR(0x37, "ADDC", 1, 1, add_handler),
	INSTR(0x38, "ADDC", 1, 1, add_handler),
	INSTR(0x39, "ADDC", 1, 1, add_handler),
	INSTR(0x3a, "ADDC", 1, 1, add_handler),
	INSTR(0x3b, "ADDC", 1, 1, add_handler),
	INSTR(0x3c, "ADDC", 1, 1, add_handler),
	INSTR(0x3d, "ADDC", 1, 1, add_handler),
	INSTR(0x3e, "ADDC", 1, 1, add_handler),
	INSTR(0x3f, "ADDC", 1, 1, add_handler),
	INSTR(0x40, "JC", 2, 2, jc_handler),
	INSTR(0x41, "AJMP", 2, 2, ajmp_handler),
	NOT_IMPLEMENTED(0x42),
	NOT_IMPLEMENTED(0x43),
	NOT_IMPLEMENTED(0x44),
	NOT_IMPLEMENTED(0x45),
	NOT_IMPLEMENTED(0x46),
	NOT_IMPLEMENTED(0x47),
	NOT_IMPLEMENTED(0x48),
	NOT_IMPLEMENTED(0x49),
	NOT_IMPLEMENTED(0x4a),
	NOT_IMPLEMENTED(0x4b),
	NOT_IMPLEMENTED(0x4c),
	NOT_IMPLEMENTED(0x4d),
	NOT_IMPLEMENTED(0x4e),
	NOT_IMPLEMENTED(0x4f),
	INSTR(0x50, "JNC", 2, 2, jnc_handler),
	INSTR(0x51, "ACALL", 2, 2, acall_handler),
	NOT_IMPLEMENTED(0x52),
	NOT_IMPLEMENTED(0x53),
	NOT_IMPLEMENTED(0x54),
	NOT_IMPLEMENTED(0x55),
	NOT_IMPLEMENTED(0x56),
	NOT_IMPLEMENTED(0x57),
	NOT_IMPLEMENTED(0x58),
	NOT_IMPLEMENTED(0x59),
	NOT_IMPLEMENTED(0x5a),
	NOT_IMPLEMENTED(0x5b),
	NOT_IMPLEMENTED(0x5c),
	NOT_IMPLEMENTED(0x5d),
	NOT_IMPLEMENTED(0x5e),
	NOT_IMPLEMENTED(0x5f),
	INSTR(0x60, "JZ", 2, 2, jz_handler),
	INSTR(0x61, "AJMP", 2, 2, ajmp_handler),
	NOT_IMPLEMENTED(0x62),
	NOT_IMPLEMENTED(0x63),
	NOT_IMPLEMENTED(0x64),
	NOT_IMPLEMENTED(0x65),
	NOT_IMPLEMENTED(0x66),
	NOT_IMPLEMENTED(0x67),
	NOT_IMPLEMENTED(0x68),
	NOT_IMPLEMENTED(0x69),
	NOT_IMPLEMENTED(0x6a),
	NOT_IMPLEMENTED(0x6b),
	NOT_IMPLEMENTED(0x6c),
	NOT_IMPLEMENTED(0x6d),
	NOT_IMPLEMENTED(0x6e),
	NOT_IMPLEMENTED(0x6f),
	INSTR(0x70, "JNZ", 2, 2, jnz_handler),
	INSTR(0x71, "ACALL", 2, 2, acall_handler),
	NOT_IMPLEMENTED(0x72),
	INSTR(0x73, "JMP", 1, 2, jmp_handler),
	NOT_IMPLEMENTED(0x74),
	NOT_IMPLEMENTED(0x75),
	NOT_IMPLEMENTED(0x76),
	NOT_IMPLEMENTED(0x77),
	NOT_IMPLEMENTED(0x78),
	NOT_IMPLEMENTED(0x79),
	NOT_IMPLEMENTED(0x7a),
	NOT_IMPLEMENTED(0x7b),
	NOT_IMPLEMENTED(0x7c),
	NOT_IMPLEMENTED(0x7d),
	NOT_IMPLEMENTED(0x7e),
	NOT_IMPLEMENTED(0x7f),
	INSTR(0x80, "SJMP", 2, 2, sjmp_handler),
	INSTR(0x81, "AJMP", 2, 2, ajmp_handler),
	NOT_IMPLEMENTED(0x82),
	INSTR(0x83, "MOVC", 1, 1, movc_pc_handler),
	NOT_IMPLEMENTED(0x84),
	NOT_IMPLEMENTED(0x85),
	NOT_IMPLEMENTED(0x86),
	NOT_IMPLEMENTED(0x87),
	NOT_IMPLEMENTED(0x88),
	NOT_IMPLEMENTED(0x89),
	NOT_IMPLEMENTED(0x8a),
	NOT_IMPLEMENTED(0x8b),
	NOT_IMPLEMENTED(0x8c),
	NOT_IMPLEMENTED(0x8d),
	NOT_IMPLEMENTED(0x8e),
	NOT_IMPLEMENTED(0x8f),
	NOT_IMPLEMENTED(0x90),
	INSTR(0x91, "ACALL", 2, 2, acall_handler),
	NOT_IMPLEMENTED(0x92),
	INSTR(0x93, "MOVC", 1, 2, movc_dptr_handler),
	NOT_IMPLEMENTED(0x94),
	NOT_IMPLEMENTED(0x95),
	NOT_IMPLEMENTED(0x96),
	NOT_IMPLEMENTED(0x97),
	NOT_IMPLEMENTED(0x98),
	NOT_IMPLEMENTED(0x99),
	NOT_IMPLEMENTED(0x9a),
	NOT_IMPLEMENTED(0x9b),
	NOT_IMPLEMENTED(0x9c),
	NOT_IMPLEMENTED(0x9d),
	NOT_IMPLEMENTED(0x9e),
	NOT_IMPLEMENTED(0x9f),
	NOT_IMPLEMENTED(0xa0),
	INSTR(0xa1, "AJMP", 2, 2, ajmp_handler),
	NOT_IMPLEMENTED(0xa2),
	NOT_IMPLEMENTED(0xa3),
	NOT_IMPLEMENTED(0xa4),
	NOT_IMPLEMENTED(0xa5),
	NOT_IMPLEMENTED(0xa6),
	NOT_IMPLEMENTED(0xa7),
	NOT_IMPLEMENTED(0xa8),
	NOT_IMPLEMENTED(0xa9),
	NOT_IMPLEMENTED(0xaa),
	NOT_IMPLEMENTED(0xab),
	NOT_IMPLEMENTED(0xac),
	NOT_IMPLEMENTED(0xad),
	NOT_IMPLEMENTED(0xae),
	NOT_IMPLEMENTED(0xaf),
	NOT_IMPLEMENTED(0xb0),
	INSTR(0xb1, "ACALL", 2, 2, acall_handler),
	NOT_IMPLEMENTED(0xb2),
	NOT_IMPLEMENTED(0xb3),
	INSTR(0xb4, "CJNE", 3, 2, cjne_a_data_handler),
	INSTR(0xb5, "CJNE", 3, 2, cjne_a_addr_handler),
	INSTR(0xb6, "CJNE", 3, 2, cjne_deref_r_data_handler),
	INSTR(0xb7, "CJNE", 3, 2, cjne_deref_r_data_handler),
	INSTR(0xb8, "CJNE", 3, 2, cjne_r_data_handler),
	INSTR(0xb9, "CJNE", 3, 2, cjne_r_data_handler),
	INSTR(0xba, "CJNE", 3, 2, cjne_r_data_handler),
	INSTR(0xbb, "CJNE", 3, 2, cjne_r_data_handler),
	INSTR(0xbc, "CJNE", 3, 2, cjne_r_data_handler),
	INSTR(0xbd, "CJNE", 3, 2, cjne_r_data_handler),
	INSTR(0xbe, "CJNE", 3, 2, cjne_r_data_handler),
	INSTR(0xbf, "CJNE", 3, 2, cjne_r_data_handler),
	NOT_IMPLEMENTED(0xc0),
	INSTR(0xc1, "AJMP", 2, 2, ajmp_handler),
	NOT_IMPLEMENTED(0xc2),
	NOT_IMPLEMENTED(0xc3),
	NOT_IMPLEMENTED(0xc4),
	NOT_IMPLEMENTED(0xc5),
	NOT_IMPLEMENTED(0xc6),
	NOT_IMPLEMENTED(0xc7),
	NOT_IMPLEMENTED(0xc8),
	NOT_IMPLEMENTED(0xc9),
	NOT_IMPLEMENTED(0xca),
	NOT_IMPLEMENTED(0xcb),
	NOT_IMPLEMENTED(0xcc),
	NOT_IMPLEMENTED(0xcd),
	NOT_IMPLEMENTED(0xce),
	NOT_IMPLEMENTED(0xcf),
	NOT_IMPLEMENTED(0xd0),
	INSTR(0xd1, "ACALL", 2, 2, acall_handler),
	NOT_IMPLEMENTED(0xd2),
	NOT_IMPLEMENTED(0xd3),
	NOT_IMPLEMENTED(0xd4),
	INSTR(0xd5, "DJNZ", 3, 2, djnz_iram_handler),
	NOT_IMPLEMENTED(0xd6),
	NOT_IMPLEMENTED(0xd7),
	INSTR(0xd8, "DJNZ", 2, 2, djnz_r_handler),
	INSTR(0xd9, "DJNZ", 2, 2, djnz_r_handler),
	INSTR(0xda, "DJNZ", 2, 2, djnz_r_handler),
	INSTR(0xdb, "DJNZ", 2, 2, djnz_r_handler),
	INSTR(0xdc, "DJNZ", 2, 2, djnz_r_handler),
	INSTR(0xdd, "DJNZ", 2, 2, djnz_r_handler),
	INSTR(0xde, "DJNZ", 2, 2, djnz_r_handler),
	INSTR(0xdf, "DJNZ", 2, 2, djnz_r_handler),
	NOT_IMPLEMENTED(0xe0),
	INSTR(0xe1, "AJMP", 2, 2, ajmp_handler),
	NOT_IMPLEMENTED(0xe2),
	NOT_IMPLEMENTED(0xe3),
	NOT_IMPLEMENTED(0xe4),
	NOT_IMPLEMENTED(0xe5),
	NOT_IMPLEMENTED(0xe6),
	NOT_IMPLEMENTED(0xe7),
	NOT_IMPLEMENTED(0xe8),
	NOT_IMPLEMENTED(0xe9),
	NOT_IMPLEMENTED(0xea),
	NOT_IMPLEMENTED(0xeb),
	NOT_IMPLEMENTED(0xec),
	NOT_IMPLEMENTED(0xed),
	NOT_IMPLEMENTED(0xee),
	NOT_IMPLEMENTED(0xef),
	NOT_IMPLEMENTED(0xf0),
	INSTR(0xf1, "ACALL", 2, 2, acall_handler),
	NOT_IMPLEMENTED(0xf2),
	NOT_IMPLEMENTED(0xf3),
	NOT_IMPLEMENTED(0xf4),
	NOT_IMPLEMENTED(0xf5),
	NOT_IMPLEMENTED(0xf6),
	NOT_IMPLEMENTED(0xf7),
	NOT_IMPLEMENTED(0xf8),
	NOT_IMPLEMENTED(0xf9),
	NOT_IMPLEMENTED(0xfa),
	NOT_IMPLEMENTED(0xfb),
	NOT_IMPLEMENTED(0xfc),
	NOT_IMPLEMENTED(0xfd),
	NOT_IMPLEMENTED(0xfe),
	NOT_IMPLEMENTED(0xff),
};
