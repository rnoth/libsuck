#include <stddef.h>
#include "test.h"
#include "../vec.h"
#include "../util.h"

// some functions here leak memory if tests fail.
// this shouldn't be a problem because you should
// fix logic errors before plugging leaks

#define test(VAR, EXPR) do { \
	int *VAR = intvec;        \
	EXPR;                     \
} while (0)

struct test {
	void (*setup)(void);
	void (*do_it)(void);
	char  *msg;
};

 
static void test_alloc(void);
static void test_clone(void);
static void test_concat(void);
static void test_copy(void);
static void test_delete(void);
static void test_elim(void);
static void test_insert(void);
static void test_fun(void);
static void test_large_insert(void);
static void test_shift(void);
static void test_slice(void);
static void test_splice(void);
static void test_transfer(void);
static void test_truncat(void);
static void cleanup(void);

struct test tests[] = {
	{ 0x0,                test_alloc,         "allocating & freeing a vector", },
	{ test_alloc,         test_insert,        "adding some elements to a simple vector", },
	{ 0x0,                test_large_insert,  "adding a large number of elements", },
	{ test_large_insert,  test_delete,        "deleting elements from a vector", },
	{ test_large_insert,  test_elim,          "eliminating elements from a vector", },
	{ test_large_insert,  test_clone,         "duplicating a vector", },
	{ test_large_insert,  test_truncat,       "truncating a vector", },
	{ test_large_insert,  test_concat,        "concatenating a vector with an array", },
	{ test_large_insert,  test_fun,           "testing fancy macros", },
	{ test_large_insert,  test_shift,         "shifting a vector", },
	{ test_large_insert,  test_copy,          "copying vectors", },
	{ test_large_insert,  test_slice,         "slicing a vector", },
	{ test_large_insert,  test_splice,        "splicing a vector with an array", },
};

static size_t const tests_len = sizeof tests / sizeof *tests;
static int *intvec;
static int *intvec2;

void
cleanup(void)
{
	if (intvec) {
		try(vec_free(intvec));
		intvec = 0x0;
	}

	if (fflush(stdout)) {
		perror("fflush");
		exit(1);
	}
}

void
test_alloc(void)
{
	ok(!vec_ctor(intvec));
	ok(vec_len(intvec) ==  0);

}

void
test_clone(void)
{
	int *cln = vec_clone(intvec);

	ok(cln);

	ok(vec_len(cln) == vec_len(intvec));
	ok(!memcmp(cln, intvec, vec_len(cln) * sizeof *cln));

	try(vec_free(cln));

}

void
test_concat(void)
{
#	define arr_len 100
	int arr[arr_len];
	int i;

	for (i = 1; i < 10 * 100; i *= 10) {
		arr[i/10] = i;
	}

	expect(0, vec_concat(&intvec, arr, arr_len));
	ok(vec_len(intvec) == 1100);

	for (i = 1; i < 10 * 100; i *= 10) {
		expect(i, intvec[1000 + i/10]);
	}

}

void
test_copy(void)
{
	int *cpy;

	expect(0, vec_ctor(cpy));

	expect(0, vec_copy(&cpy, intvec));
	expect(1000, vec_len(cpy));

	ok(!memcmp(cpy, intvec, 1000 * sizeof *cpy));

	expect(0, vec_copy(&intvec, cpy));
	expect(1000, vec_len(intvec));

	ok(!memcmp(cpy, intvec, 1000 * sizeof *cpy));
	try(vec_free(cpy));

}

void
test_delete(void)
{
	for (int i = 501; i < 600; ++i) {
		try(vec_delete(&intvec, 500));
		expect(900U + 600U - i, vec_len(intvec));
		ok(intvec[500] == i);
	}

}

void
test_elim(void)
{
	try(vec_elim(&intvec, 200, 400));
	expect(600, vec_len(intvec));

	for (int i = 0; i < 200; ++i) {
		ok(intvec[i] == i);
	}

	for (int i = 200; i < 600; ++i) {
		ok(intvec[i] == i + 400);
	}

}

void
test_insert(void)
{
	expect(0, vec_append(&intvec, (int[]){5}));
	ok(vec_len(intvec) == 1);
	ok(intvec[0] == 5);

	expect(0, vec_insert(&intvec, (int[]){3}, 0));
	ok(len(intvec) == 2);
	ok(intvec[0] == 3);
	ok(intvec[1] == 5);

	expect(0, vec_insert(&intvec, (int[]){312345}, 1));
	ok(len(intvec) == 3);
	ok(intvec[0] == 3);
	ok(intvec[1] == 312345);
	ok(intvec[2] == 5);

	expect(0, vec_insert(&intvec, (int[]){15557145}, 1));
	ok(len(intvec) == 4);
	ok(intvec[0] == 3);
	ok(intvec[1] == 15557145);
	ok(intvec[2] == 312345);
	ok(intvec[3] == 5);

}

void
test_fun(void)
{
	// FIXME: does not test nested loops
	int i;
	printf("\t\ttesting vec_foreach()...\n");

	i = 0;
	vec_foreach (int *each, intvec) expect(*each, i++);

	printf("\t\tretrying without a declaration...\n");

	i = 0;
	int *each;
	vec_foreach (each, intvec) expect(*each, i++);

	printf("\t\ttrying continue inside vec_foreach()...\n");
	vec_foreach (int *i, intvec) {
		if (*i != 8) continue;
		try(vec_delete(&intvec, i - intvec)); // note: don't try this at home
	}

	expect(999, vec_len(intvec));
	expect(7, intvec[7]);
	expect(9, intvec[8]);

	printf("\t\ttrying to break out of vec_foreach()...\n");
	vec_foreach (int *i, intvec) {
		if (*i == 9) break;
		ok(*i < 9);
	}

}

void
test_large_insert(void)
{
	ok(!vec_ctor(intvec));

	for (int i = 0; i < 1000; ++i) {
		expect(0, vec_append(&intvec, &i));
		ok(intvec[i] == i);
		ok(vec_len(intvec) == (size_t)i + 1);
	}

}

void
test_shift(void)
{
	int i;

	try(vec_shift(&intvec, 250));
	for (i = 0; i < 750; ++i) expect(i + 250, intvec[i]);

}

void
test_slice(void)
{
	int i;

	try(vec_slice(&intvec, 300, 600));

	expect(600, vec_len(intvec));

	for (i = 0; i < 600; ++i) {
		expect(i + 300, intvec[i]);
	}

}

void
test_splice(void)
{
#	define arr_len 100
	int i;
	int arr[arr_len];

	for (i = 0; i < 100; ++i) arr[i] = intvec[i + 600];

	expect(0, vec_splice(&intvec, 400, arr, arr_len));
	expect(1100, vec_len(intvec));

	for (i = 0; i < 400; ++i) expect(i, intvec[i]);
	ok(!memcmp(intvec + 400, arr, arr_len));
	for (i = 0; i < 600; ++i) expect(i + 400, intvec[i + 500]);

}

void
test_transfer(void)
{
#	define arr_len 100
	int arr[arr_len];
	int i;

	for (i = 0; i < 100; ++i) arr[i] = intvec[i + 600];

	expect(0, vec_transfer(&intvec, arr, arr_len));
	expect(100, vec_len(intvec));

	ok(!memcmp(intvec, arr, 1000 * sizeof *intvec));

}

void
test_truncat(void)
{
	try(vec_truncat(&intvec, 500));
	ok(vec_len(intvec) == 500);

	try(vec_truncat(&intvec, 0));
	ok(vec_len(intvec) == 0);
}

int
main()
{
	size_t i;

	init_test();

	printf("testing vec.c\n");

	for (i = 0; i < tests_len; ++i) {
		on_failure {
			cleanup();
			continue;
		}

		if (tests[i].setup) tests[i].setup();
		printf("\t%s...\n", tests[i].msg);
		if (tests[i].do_it) tests[i].do_it();

		cleanup();
	}

	printf("testing finished (vec.c) -- %zu failed tests\n", total_failures);

	return 0;
}
