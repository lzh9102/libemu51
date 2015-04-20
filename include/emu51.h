#ifndef _EMU51_H_
#define _EMU51_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Additional features settings of the emulator.
 *
 * Set the respective bits to enable the features.
 */
typedef struct emu51_features
{
	unsigned int timer2:1; /**< has timer2 */
} emu51_features;

/* forward declaration: used in struct emu51 */
typedef struct emu51_callbacks emu51_callbacks;

/** 8051/8052 emulator structure
 *
 * This structure holds the state of the emulator.
 */
typedef struct emu51
{
	const uint8_t *pmem; /**< Read-only program memory */
	uint16_t pmem_len; /**< Size of the @c pmem buffer,
								must be power of 2 within 1k~64k */

	/** Lower internal memory (address 0~127).
	 *
	 * User must supply a 128-byte buffer to this pointer.
	 */
	uint8_t *iram_lower;

	/** Upper internal memory (address 128~255).
	 *
	 * The size of the buffer should be 128 bytes. Leave this field NULL if
	 * unused (8051 mode).
	 */
	uint8_t *iram_upper;

	/** Buffer to store special function registers (SFR).
	 *
	 * User must supply a 128-byte buffer to this pointer.
	 */
	uint8_t *sfr;

	uint8_t *xram; /**< External memory, leave it NULL if not used */
	uint16_t xram_len; /**< Size of the @c xram buffer,
								must be power of 2 within 1k~64k */

	uint16_t pc; /**< Program counter */

	emu51_features features; /**< Additional features of the emulator. */

	emu51_callbacks *callbacks; /**< structure to store callback pointers,
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

/** Indices of SFRs in the sfr buffer (@c emu51.sfr). */
enum emu51_sfr_index
{
	/* The SFR buffer is 128 bytes, but the memory-mapped address of the SFRs
	 * are in the range 0x80~0xff. Therefore, the indices of the SFRs are 0x80
	 * subtracted from their addresses.
	 */
	SFR_P0 = 0x80 - 0x80,   /**< I/O port 0 */
	SFR_P1 = 0x90 - 0x80,   /**< I/O port 1 */
	SFR_P2 = 0xa0 - 0x80,   /**< I/O port 2 */
	SFR_P3 = 0xb0 - 0x80,   /**< I/O port 3 */

	SFR_SP = 0x81 - 0x80,   /**< stack pointer */
	SFR_DPL = 0x82 - 0x80,  /**< data pointer low */
	SFR_DPH = 0x83 - 0x80,  /**< data pointer high */
	SFR_PCON = 0x87 - 0x80, /**< power control */

	SFR_TCON = 0x88 - 0x80, /**< timer control */
	SFR_TMOD = 0x89 - 0x80, /**< timer mode */
	SFR_TL0 = 0x8a - 0x80,  /**< timer 0 low */
	SFR_TH0 = 0x8c - 0x80,  /**< timer 0 high */
	SFR_TL1 = 0x8b - 0x80,  /**< timer 1 low */
	SFR_TH1 = 0x8d - 0x80,  /**< timer 1 high */

	SFR_SCON = 0x98 - 0x80, /**< serial control */
	SFR_SBUF = 0x99 - 0x80, /**< serial buffer */

	SFR_IE = 0xa8 - 0x80,   /**< interrupt enable */
	SFR_IP = 0xb8 - 0x80,   /**< interrupt priority */

	SFR_PSW = 0xd0 - 0x80,  /**< program status word */
	SFR_ACC = 0xe0 - 0x80,  /**< accumulator */
	SFR_B = 0xf0 - 0x80,    /**< B register */
};

/* bitmasks of PSW */
enum emu51_psw_mask
{
	PSW_P = 0x01,   /* parity */
	PSW_UD = 0x02,  /* user defined, for general software use */
	PSW_OV = 0x04,  /* overflow flag */
	PSW_RS0 = 0x08, /* lower-order bit of register selection */
	PSW_RS1 = 0x10, /* higher-order bit of register selection */
	PSW_F0 = 0x20,  /* flag 0, for general software use */
	PSW_AC = 0x40,  /* auxiliary carry */
	PSW_C  = 0x80,  /* carry */
};

enum emu51_errno
{
	EMU51_SUCCESS = 0,
	EMU51_PMEM_OUT_OF_RANGE,
};

/** Reset the emulator.
 *
 * @note The SFR buffer @c m->sfr must be specified before calling this
 * function.
 *
 * @param m the emulator struct
 */
void emu51_reset(emu51 *m);

/** Execute one instruction.
 *
 * @param m the emulator object
 * @param cycles [out] How many cycles does the instruction takes. Set it to
 *                     NULL to ignore the value.
 *
 * @return Returns 0 if success; returns an error number on failure.
 */
int emu51_step(emu51 *m, int *cycles);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _EMU51_H_ */
