/* unit tests for the instructions */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>
#include <stdlib.h>

#include <emu51.h>

/* library internal headers */
#include <instr.h>

/* Replace memory allocation functions with cmocka's version, which can detect
 * memory leak and buffer overflow bugs.
 */
#define malloc test_malloc
#define calloc test_calloc
#define free test_free

/* data used in unit tests */
typedef struct testdata
{
	/* each buffer is allocated separately to detect buffer overflow */
	emu51 *m; /* the emulator struct */
	uint8_t *sfr; /* buffer to hold SFRs, 128 bytes */
	uint8_t *pmem; /* program memory */
	uint16_t pmem_len; /* program memory size */
	uint8_t *iram_lower; /* lower internal RAM, 128 bytes */
	uint8_t *iram_upper; /* lower internal RAM, 128 bytes */
	uint8_t *xram; /* external RAM */
	uint16_t xram_len; /* size of the external RAM */
} testdata;

/* Create emulator and buffers for testing.
 *
 * Do not share the created test data across tests because this will make it
 * difficult to locate the test that caused memory bugs. Instead, allocate the
 * data in each test and free it after use.
 */
testdata *alloc_test_data()
{
	testdata *data = calloc(1, sizeof(testdata));

	/* emulator struct */
	data->m = calloc(1, sizeof(emu51));

	/* special function register */
	data->sfr = calloc(128, 1);

	/* internal RAM */
	data->iram_lower = calloc(128, 1);
	data->iram_upper = calloc(128, 1);

	/* 4KB of program memory */
	data->pmem_len = 4 * 1024;
	data->pmem = calloc(data->pmem_len, 1);

	/* 32KB of external RAM */
	data->xram_len = 32 * 1024;
	data->xram = calloc(data->xram_len, 1);

	/* setup the emulator */
	memset(data->m, 0, sizeof(emu51));
	data->m->sfr = data->sfr;
	data->m->pmem = data->pmem;
	data->m->pmem_len = data->pmem_len;
	data->m->iram_lower = data->iram_lower;
	data->m->iram_upper = data->iram_upper;
	data->m->xram = data->xram;
	data->m->xram_len = data->xram_len;
	emu51_reset(data->m);

	return data;
}

/* free test data allocated by alloc_test_data() */
void free_test_data(testdata *data)
{
	free(data->m);
	free(data->sfr);
	free(data->pmem);
	free(data->iram_lower);
	free(data->iram_upper);
	free(data->xram);
	free(data);
}

/* here goes the test functions */

/* TODO: add tests here */

/* end of test functions */

int main()
{
	const struct CMUnitTest tests[] = {
		/* TODO: add tests here */
	};
	/* don't use setup and teardown as cmocka doesn't report memory bugs in them
	 */
	return cmocka_run_group_tests(tests, NULL, NULL);
}
