#include <emu51.h>
#include <assert.h>

/* convenient macro to access registers
 *
 * example: use REG(m, ACC) to access the accumulator
 */
#define REG(m, reg_name) (*(&m->iram[ADDR_ ## reg_name]))

void emu51_reset(emu51 *m)
{
	assert(m->iram && "emu51_reset: m->iram must not be NULL");

	m->pc = 0;
	REG(m, SP) = 0x07;
}
