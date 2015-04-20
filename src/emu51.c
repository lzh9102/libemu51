#include <emu51.h>
#include <assert.h>
#include "helpers.h"
#include "instr.h"

void emu51_reset(emu51 *m)
{
	assert(m->sfr && "emu51_reset: m->sfr must not be NULL");

	m->pc = 0;
	m->errno = 0;
	REG(m, SP) = 0x07;
}

int emu51_step(emu51 *m, int *cycles)
{
	/* do not continue if there is an error */
	if (m->errno)
		return m->errno;

	/* check if pc is in range */
	if (m->pc >= m->pmem_len) {
		m->errno = EMU51_PMEM_OUT_OF_RANGE;
		return m->errno;
	}

	/* decode the instruction */
	const uint8_t *code = &m->pmem[m->pc];
	const emu51_instr *instr = _emu51_get_instr(code[0]);

	/* do not execute multi-byte instruction if it crosses the boundary */
	if (m->pc + instr->bytes > m->pmem_len) {
		m->errno = EMU51_PMEM_OUT_OF_RANGE;
		return m->errno;
	}

	/* increment pc */
	m->pc += instr->bytes;

	/* invoke instruction handler */
	int errno = instr->handler(instr, code, m);
	if (errno) {
		m->errno = errno;
		return m->errno;
	}

	/* return the cycle count of the instruction */
	if (cycles)
		*cycles = instr->cycles;

	return 0;
}
