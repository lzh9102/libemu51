#include <emu51.h>
#include "instr.h"

/* Helper functions */

/* Read data from immediate address.
 * An immediate address can refer to:
 *	 1. internal ram, if addr < 0x80
 *	 2. SFR, if addr >= 0x80
 */
static inline uint8_t direct_addr_read(emu51 *m, uint8_t addr)
{
	if (addr < 0x80) /* lower internal ram (0~0x7f) */
		return m->iram_lower[addr];
	else /* SFR */
		return m->sfr[addr - SFR_BASE_ADDR];
}

/* Read data from the address pointed by ptr.
 * The indirect address always refer to internal ram, not SFR.
 * Returns 0 on success or return an error code. The caller must check for the
 * error code.
 */
static inline int indirect_addr_read(emu51 *m, uint8_t ptr, uint8_t *out)
{
	uint8_t addr = direct_addr_read(m, ptr);
	if (addr < 0x80) { /* lower iram */
		/* m->iram_lower is required to be set by the user, so we don't have to
		 * check for NULL here. */
		*out = m->iram_lower[addr];
		return 0;
	} else { /* upper iram */
		/* m->iram_upper is not guaranteed to exist, so checking is necessary */
		if (m->iram_upper) {
			*out = m->iram_upper[addr - 0x80];
			return 0;
		} else { /* upper iram is not set */
			return EMU51_IRAM_OUT_OF_RANGE;
		}
	}
}

/* Implementations of 8051/8052 instructions.
 *
 * The functions should return 0 on success or an error code on error.
 */

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

/* operation: LJMP
 * function: long jump
 */
DEFINE_HANDLER(ljmp_handler)
{
	uint16_t target_addr = (OPERAND1 << 8) | OPERAND2;
	PC = target_addr;
	return 0;
}

/* operation: SJMP
 * function: relative jump -128~+127 bytes
 */
DEFINE_HANDLER(sjmp_handler)
{
	int8_t reladdr = OPERAND1;

	/* FIXME: what happens when jumping across program memory border
	 *	       (e.g. PC == 0, reladdr == -1)?
	 * 1. wraps over? [current implementation]
	 * 2. error?
	 */
	PC += reladdr;

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
		uint8_t reladdr)
{
	/* compute carry flag: set only when op1 < op2 */
	if (op1 < op2)
		PSW |= PSW_C;
	else
		PSW &= ~PSW_C;

	/* branch if not equal */
	if (op1 != op2)
		PC += reladdr;

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
	uint8_t reladdr = OPERAND2;

	return general_cjne(m, ACC, data, reladdr);
}

/* operation: CJNE A, iram addr, reladdr
 * flags: carry
 */
DEFINE_HANDLER(cjne_a_addr_handler)
{
	uint8_t addr = OPERAND1;
	uint8_t reladdr = OPERAND2;
	uint8_t data = direct_addr_read(m, addr);

	return general_cjne(m, ACC, data, reladdr);
}

/* operation: CJNE @R0, #data, reladdr
 * flags: carry
 */
DEFINE_HANDLER(cjne_deref_r_data_handler)
{
	uint8_t data = OPERAND1;
	uint8_t reladdr = OPERAND2;

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
	uint8_t reladdr = OPERAND2;

	/* The last 3 bit of opcode is the R number (0~7) */
	uint8_t r_index = OPCODE & 0x07;

	return general_cjne(m, REG_R(r_index), data, reladdr);
}

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
	NOT_IMPLEMENTED(0x10),
	NOT_IMPLEMENTED(0x11),
	NOT_IMPLEMENTED(0x12),
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
	NOT_IMPLEMENTED(0x20),
	INSTR(0x21, "AJMP", 2, 2, ajmp_handler),
	NOT_IMPLEMENTED(0x22),
	NOT_IMPLEMENTED(0x23),
	NOT_IMPLEMENTED(0x24),
	NOT_IMPLEMENTED(0x25),
	NOT_IMPLEMENTED(0x26),
	NOT_IMPLEMENTED(0x27),
	NOT_IMPLEMENTED(0x28),
	NOT_IMPLEMENTED(0x29),
	NOT_IMPLEMENTED(0x2a),
	NOT_IMPLEMENTED(0x2b),
	NOT_IMPLEMENTED(0x2c),
	NOT_IMPLEMENTED(0x2d),
	NOT_IMPLEMENTED(0x2e),
	NOT_IMPLEMENTED(0x2f),
	NOT_IMPLEMENTED(0x30),
	NOT_IMPLEMENTED(0x31),
	NOT_IMPLEMENTED(0x32),
	NOT_IMPLEMENTED(0x33),
	NOT_IMPLEMENTED(0x34),
	NOT_IMPLEMENTED(0x35),
	NOT_IMPLEMENTED(0x36),
	NOT_IMPLEMENTED(0x37),
	NOT_IMPLEMENTED(0x38),
	NOT_IMPLEMENTED(0x39),
	NOT_IMPLEMENTED(0x3a),
	NOT_IMPLEMENTED(0x3b),
	NOT_IMPLEMENTED(0x3c),
	NOT_IMPLEMENTED(0x3d),
	NOT_IMPLEMENTED(0x3e),
	NOT_IMPLEMENTED(0x3f),
	NOT_IMPLEMENTED(0x40),
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
	NOT_IMPLEMENTED(0x50),
	NOT_IMPLEMENTED(0x51),
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
	NOT_IMPLEMENTED(0x60),
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
	NOT_IMPLEMENTED(0x70),
	NOT_IMPLEMENTED(0x71),
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
	NOT_IMPLEMENTED(0x91),
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
	NOT_IMPLEMENTED(0xb1),
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
	NOT_IMPLEMENTED(0xd1),
	NOT_IMPLEMENTED(0xd2),
	NOT_IMPLEMENTED(0xd3),
	NOT_IMPLEMENTED(0xd4),
	NOT_IMPLEMENTED(0xd5),
	NOT_IMPLEMENTED(0xd6),
	NOT_IMPLEMENTED(0xd7),
	NOT_IMPLEMENTED(0xd8),
	NOT_IMPLEMENTED(0xd9),
	NOT_IMPLEMENTED(0xda),
	NOT_IMPLEMENTED(0xdb),
	NOT_IMPLEMENTED(0xdc),
	NOT_IMPLEMENTED(0xdd),
	NOT_IMPLEMENTED(0xde),
	NOT_IMPLEMENTED(0xdf),
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
	NOT_IMPLEMENTED(0xf1),
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
