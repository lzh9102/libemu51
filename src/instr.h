#ifndef _INSTR_H_
#define _INSTR_H_

/* NOTE: This header file is internal to emu51. */

#include <stdint.h>

/* forward declarations */
typedef struct emu51 emu51;
typedef struct emu51_instr emu51_instr;

/* instruction handler
 *
 * instr: the emu51_instr struct containing information about the instruction
 * code: the actual bytes of the instruction,
 *       its size must be at least instr->bytes
 * m: the emulator structure
 */
typedef int (*instr_handler)(emu51_instr *instr, const uint8_t *code, emu51* m);

typedef struct emu51_instr
{
	uint8_t opcode;

	unsigned int bytes:4;  /* length of the instruction in bytes,
	                          zero if the instruction is not implemented */
	unsigned int cycles:4; /* number of machine cycles the instruction takes */

	instr_handler handler; /* callback function to process the instruction */
} emu51_instr;

/* find the instruction by opcode */
static inline const emu51_instr* emu51_get_instr(uint8_t opcode)
{
	extern const emu51_instr _emu51_instr_table[];
	return &_emu51_instr_table[opcode];
}

#endif /* _INSTR_H_ */
