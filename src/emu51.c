#include <emu51.h>
#include <assert.h>

#include "instr.h"

void emu51_reset(emu51 *m)
{
	assert(m->sfr && "emu51_reset: m->sfr must not be NULL");

	m->pc = 0;
	m->sfr[SFR_SP] = 0x07; /* initial stack pointer in 8051 is 0x07 */
}

int emu51_step(emu51 *m, int *cycles)
{
	/* check if pc points to a valid program memory location */
	if (m->pc >= m->pmem_len)
		return EMU51_PMEM_OUT_OF_RANGE;

	/* decode the instruction */
	const uint8_t *code = &m->pmem[m->pc];
	const emu51_instr *instr = _emu51_decode_instr(code[0]);

	/* check if the entire instruction resides in valid program memory */
	if (m->pc + instr->bytes > m->pmem_len)
		return EMU51_PMEM_OUT_OF_RANGE;

	/* Increment pc and save the old pc in case an error occurs.
	 * FIXME: does the pc wrap around at the end of program memory?
	 */
	uint16_t old_pc = m->pc;
	m->pc += instr->bytes;

	/* invoke instruction handler */
	int instr_error = instr->handler(instr, code, m);
	if (instr_error) {
		m->pc = old_pc; /* restore pc when an error occurs */
		return instr_error;
	}

	/* return the cycle count of the instruction */
	if (cycles)
		*cycles = instr->cycles;

	return 0;
}
