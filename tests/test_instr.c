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

#define PMEM_SIZE 4096         /* program memory size used in tests */
#define XRAM_SIZE (32 * 1024)  /* external memory size used in tests */

/* data used in unit tests */
typedef struct testdata
{
	/* each buffer is allocated separately to detect buffer overflow */
	emu51 *m; /* the emulator struct */
	uint8_t *sfr; /* buffer to hold SFRs, 128 bytes */
	uint8_t *pmem; /* program memory */
	uint8_t *iram_lower; /* lower internal RAM, 128 bytes */
	uint8_t *iram_upper; /* lower internal RAM, 128 bytes */
	uint8_t *xram; /* external RAM */
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
	data->pmem = calloc(PMEM_SIZE, 1);

	/* external RAM */
	data->xram = calloc(XRAM_SIZE, 1);

	/* setup the emulator */
	memset(data->m, 0, sizeof(emu51));
	data->m->sfr = data->sfr;
	data->m->pmem = data->pmem;
	data->m->pmem_len = PMEM_SIZE;
	data->m->iram_lower = data->iram_lower;
	data->m->iram_upper = data->iram_upper;
	data->m->xram = data->xram;
	data->m->xram_len = XRAM_SIZE;
	emu51_reset(data->m);

	return data;
}

/* Duplicate the data (make a separate copy of it).
 *
 * It's the caller's responsibility to free the duplicated data.
 */
testdata *dup_test_data(const testdata *orig)
{
	testdata *data = alloc_test_data();

	/* copy the emulator fields
	 * ATTENTION: pointers like pmem and iram are overwritten by the pointers
	 * in the orig struct. It is necessary to fix this.
	 */
	*data->m = *orig->m;
	/* fix the pointer fields */
	data->m->sfr = data->sfr;
	data->m->pmem = data->pmem;
	data->m->iram_lower = data->iram_lower;
	data->m->iram_upper = data->iram_upper;
	data->m->xram = data->xram;

	/* copy the content of buffers */
	memcpy(data->sfr, orig->sfr, 128);
	memcpy(data->pmem, orig->pmem, PMEM_SIZE);
	memcpy(data->iram_lower, orig->iram_lower, 128);
	memcpy(data->iram_upper, orig->iram_upper, 128);
	memcpy(data->xram, orig->m->xram, XRAM_SIZE);

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

/* write random data to the buffer */
void write_random_data(uint8_t *buffer, size_t size)
{
	int i;
	for (i = 0; i < size; i++)
		buffer[i] = rand() & 0xff;
}

/* write random data to the memories (pmem, iram, xram) in the testdata */
void write_random_data_to_memories(testdata *data)
{
	write_random_data(data->pmem, PMEM_SIZE);
	write_random_data(data->sfr, 128);
	write_random_data(data->iram_lower, 128);
	write_random_data(data->iram_upper, 128);
	write_random_data(data->xram, XRAM_SIZE);
}

#define assert_emu51_sfr_equal(data1, data2) \
	assert_memory_equal(data1->sfr, data2->sfr, 128)

#define assert_emu51_iram_equal(data1, data2) do { \
	assert_memory_equal(data1->iram_lower, data2->iram_lower, 128); \
	assert_memory_equal(data1->iram_upper, data2->iram_upper, 128); } while (0)

#define assert_emu51_xram_equal(data1, data2) \
	assert_memory_equal(data1->xram, data2->xram, XRAM_SIZE)

/* sfr, iram, xram all equal */
#define assert_emu51_all_ram_equal(data1, data2) do { \
	assert_emu51_sfr_equal(data1, data2); \
	assert_emu51_iram_equal(data1, data2); \
	assert_emu51_xram_equal(data1, data2); } while (0)

/* here goes the test functions */

void test_nop(void **state)
{
	testdata *data = alloc_test_data();
	emu51 *m = data->m;

	/* fill in random data to emulator memory */
	write_random_data_to_memories(data);

	/* fill in NOP in pmem[0] */
	data->pmem[0] = 0x00;

	/* save the original state */
	testdata *orig_data = dup_test_data(data);

	/* execute nop */
	const emu51_instr *instr = emu51_get_instr(0x00); /* NOP */
	instr->handler(instr, &m->pmem[0], m);

	/* nop shouldn't change any of the SFRs, iram or xram */
	assert_emu51_all_ram_equal(data, orig_data);

	free_test_data(orig_data);
	free_test_data(data);
}

/* end of test functions */

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_nop),
	};
	/* don't use setup and teardown as cmocka doesn't report memory bugs in them
	 */
	return cmocka_run_group_tests(tests, NULL, NULL);
}
