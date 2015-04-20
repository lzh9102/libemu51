#include <emu51.h>
#include <assert.h>
#include "helpers.h"
#include "instr.h"

void emu51_reset(emu51 *m)
{
	assert(m->sfr && "emu51_reset: m->sfr must not be NULL");

	m->pc = 0;
	REG(m, SP) = 0x07;
}

int emu51_step(emu51 *m, int *cycles)
{
	/* check if pc points to a valid program memory location */
	if (m->pc >= m->pmem_len)
		return EMU51_PMEM_OUT_OF_RANGE;

	/* decode the instruction */
	const uint8_t *code = &m->pmem[m->pc];
	const emu51_instr *instr = _emu51_get_instr(code[0]);

	/* check if the entire instruction resides in valid program memory */
	if (m->pc + instr->bytes > m->pmem_len)
		return EMU51_PMEM_OUT_OF_RANGE;

	/* increment pc */
	m->pc += instr->bytes;

	/* invoke instruction handler */
	int instr_error = instr->handler(instr, code, m);
	if (instr_error)
		return instr_error;

	/* return the cycle count of the instruction */
	if (cycles)
		*cycles = instr->cycles;

	return 0;
}
