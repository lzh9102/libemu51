/* memory accessing functions */

#ifndef _MEM_H_
#define _MEM_H_

#include <emu51.h>

#define BIT_ADDR_BASE 0x20

/* Read data from immediate address.
 * An immediate address can refer to:
 *  1. internal ram, if addr < 0x80
 *  2. SFR, if addr >= 0x80
 */
static inline uint8_t direct_addr_read(emu51 *m, uint8_t addr)
{
	if (addr < 0x80) /* lower internal ram (0~0x7f) */
		return m->iram_lower[addr];
	else /* SFR */
		return m->sfr[addr - SFR_BASE_ADDR];
}

/* Write data to direct address */
static inline void direct_addr_write(emu51 *m, uint8_t addr, uint8_t data)
{
	if (addr < 0x80) /* lower internal ram (0~0x7f) */
		m->iram_lower[addr] = data;
	else /* SFR */
		m->sfr[addr - SFR_BASE_ADDR] = data;
}

/* Read data from the address obtained by dereferencing ptr.
 *
 * ptr: address of an iram byte (ptr < 0x80) or a SFR (ptr >= 0x80).
 *
 * Example: If ptr == 0xe0 (address of ACC) and the content of ACC is 0x30,
 * the content of address 0x30 will be written to *out.
 *
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

/* Write data to the address obtained by dereferencing ptr.
 * See indirect_addr_read for details.
 */
static inline int indirect_addr_write(emu51 *m, uint8_t ptr, uint8_t data)
{
	uint8_t addr = direct_addr_read(m, ptr);
	if (addr < 0x80) { /* lower iram */
		/* m->iram_lower is required to be set by the user, so we don't have to
		 * check for NULL here. */
		m->iram_lower[addr] = data;
		return 0;
	} else { /* upper iram */
		/* m->iram_upper is not guaranteed to exist, so checking is necessary */
		if (m->iram_upper) {
			m->iram_upper[addr - 0x80] = data;
			return 0;
		} else { /* upper iram is not set */
			return EMU51_IRAM_OUT_OF_RANGE;
		}
	}
}

/* Read bit memory.
 *
 * There are 128 bit variables located from 0x20 through 0x2f (16 bytes in
 * total).
 *
 * Returns the bit value (0 or 1) on success, negative error code on error.
 */
static inline int bit_read(emu51 *m, uint8_t addr)
{
	if (addr < 0x80) {
		int byte_offset = addr / 8;
		int bit_index = addr % 8;
		uint8_t bitmap = (1 << bit_index);
		uint8_t byte_value = m->iram_lower[BIT_ADDR_BASE + byte_offset];
		return (byte_value & bitmap) ? 1 : 0;
	} else { /* bit address >= 128 is invalid */
		return EMU51_BIT_OUT_OF_RANGE;
	}
}

/* Push a value onto the stack. The SP is first incremented, and
 * the data is then written to the position pointed by the new SP.
 * Returns 0 on success or EMU51_IRAM_OUT_OF_RANGE on stack overflow.
 */
static inline int stack_push(emu51 *m, uint8_t data)
{
	++m->sfr[SFR_SP];
	return indirect_addr_write(m, SFR_BASE_ADDR + SFR_SP, data);
}

/* Add reladdr to the program counter.
 * reladdr: signed 8-bit offset
 */
static inline void relative_jump(emu51 *m, int8_t reladdr)
{
	/* FIXME: what happens when jumping across program memory border
	 *        (e.g. PC == 0, reladdr == -1)?
	 * 1. wraps over? [current implementation]
	 * 2. error?
	 */
	m->pc += reladdr;
}

#endif /* _MEM_H_ */
