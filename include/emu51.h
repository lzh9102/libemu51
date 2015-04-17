#ifndef _EMU51_H_
#define _EMU51_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct emu51_callbacks; /* forward declaration: used in struct emu51 */

/** 8051/8052 emulator structure
 *
 * This structure holds the state of the emulator.
 */
typedef struct emu51
{
	const uint8_t *pmem; /**< Read-only program memory */
	uint16_t pmem_len; /**< Size of the @c pmem buffer,
								must be power of 2 within 1k~64k */

	uint8_t *iram; /**< Internal memory */
	uint16_t iram_len; /**< Size of the @c iram buffer,
								must be power of 2 within 1k~64k */

	uint8_t *xram; /**< External memory, leave it NULL if not used */
	uint16_t xram_len; /**< Size of the @c xram buffer,
								must be power of 2 within 1k~64k */

	uint16_t pc; /**< Program counter */

	struct emu51_callbacks *callbacks; /**< structure to store callback pointers,
													 leave it NULL if not used */

	/** Pointer for the user to store arbitrary data.
	 *
	 * This pointer can be used to store extra data associated with the emulator
	 * (e.g. c++ class instance); emu51 doesn't touch this pointer at all.
	 */
	void *userdata;
} emu51;

/** emulator callbacks
 *
 * This structure stores callback pointers.
 */
typedef struct emu51_callbacks
{
} emu51_callbacks;

/* addresses of SFRs */
enum emu51_sfr_addrs
{
	ADDR_P0 = 0x80,   /* I/O port 0 */
	ADDR_P1 = 0x90,   /* I/O port 1 */
	ADDR_P2 = 0xa0,   /* I/O port 2 */
	ADDR_P3 = 0xb0,   /* I/O port 3 */

	ADDR_SP = 0x81,   /* stack pointer */
	ADDR_DPL = 0x82,  /* data pointer low */
	ADDR_DPH = 0x83,  /* data pointer high */
	ADDR_PCON = 0x87, /* power control */

	ADDR_TCON = 0x88, /* timer control */
	ADDR_TMOD = 0x89, /* timer mode */
	ADDR_TL0 = 0x8a,  /* timer 0 low */
	ADDR_TH0 = 0x8c,  /* timer 0 high */
	ADDR_TL1 = 0x8b,  /* timer 1 low */
	ADDR_TH1 = 0x8d,  /* timer 1 high */

	ADDR_SCON = 0x98, /* serial control */
	ADDR_SBUF = 0x99, /* serial buffer */

	ADDR_IE = 0xa8,   /* interrupt enable */
	ADDR_IP = 0xb8,   /* interrupt priority */

	ADDR_PSW = 0xD0,  /* program status word */
	ADDR_ACC = 0xe0,  /* accumulator */
	ADDR_B = 0xf0,    /* B register, used in multiply and divide operations */
};

/* bit definitions of PSW */
enum emu51_psw_bits
{
	PSW_P = 0,    /* parity */
	PSW_UD = 1,   /* user defined, for general software use */
	PSW_OV = 2,   /* overflow flag */
	PSW_RS0 = 3,  /* lower-order bit of register selection */
	PSW_RS1 = 4,  /* higher-order bit of register selection */
	PSW_F0 = 5,   /* flag 0, for general software use */
	PSW_AC = 6,   /* auxiliary carry */
	PSW_C  = 7,   /* carry */
};

/** Reset the emulator.
 *
 * @note The internal ram @c m->iram and @c m->iram_len
 * must be initialized before calling this function.
 *
 * @param m the emulator struct
 */
void emu51_reset(emu51 *m);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _EMU51_H_ */
