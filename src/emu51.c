#include <emu51.h>
#include <assert.h>
#include "helpers.h"

void emu51_reset(emu51 *m)
{
	assert(m->sfr && "emu51_reset: m->sfr must not be NULL");

	m->pc = 0;
	m->errno = 0;
	REG(m, SP) = 0x07;
}
