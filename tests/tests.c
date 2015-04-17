#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include <emu51.h>

void test_reset(void** state)
{
	int i;
	emu51 m;
	uint8_t iram[0xff];

	/* fill the memory with gibberish to see if reset really works */
	for (i = 0; i < sizeof(iram); i++)
		iram[i] = 0xaa;

	/* initialize the emulator struct */
	memset(&m, 0, sizeof(m));
	m.iram = iram;
	m.iram_len = sizeof(iram);
	m.pc = 0x10; /* set pc to arbitrary value to test the reset */

	emu51_reset(&m);
	assert_int_equal(m.pc, 0);
	assert_int_equal(m.iram[ADDR_SP], 0x07); /* initial value of SP is 07h */

}

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_reset),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
