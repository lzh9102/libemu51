#ifndef _HELPERS_H_
#define _HELPERS_H_

/* Access register reg_name in emulator m
 *
 * example: use REG(m, ACC) to access the accumulator
 */
#define REG(m, reg_name) ((m)->sfr[SFR_ ## reg_name])

#endif /* _HELPERS_H_ */
