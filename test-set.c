#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <unit.h>
#include <set.h>
#include <util.h>

static void test_alloc(void);
static void test_add(void);
static void test_free(void);
static void test_query(void);
static void test_remove(void);
static void test_fixed(void);
static void test_prefix(void);
static void test_large_add(void);
static void test_dup(void);

char unit_filename[] = "set.c"; 

struct test unit_tests[] = {
	{ "allocating a set",                    0x0,         test_alloc,     test_free, },
	{ "inserting a string",                  0x0,         test_add,       test_free, },
	{ "querying the set",                    test_add,    test_query,     test_free, },
	{ "removing an element",                 test_add,    test_remove,    test_free, },
	{ "querying with a fixed-size buffer",   test_add,    test_fixed,     test_free, },
	{ "testing the prefix check",            test_add,    test_prefix,    test_free, },
	{ "adding a large number of strings",    test_alloc,  test_large_add, test_free, },
	{ "attempting to add duplicate strings", test_add,    test_dup,       test_free, },
	{ 0x0 },
};

struct set *set;
char **reply = 0x0;

char *strings[] = { "foo", "bar", "baz", "quux", };

void
test_free(void)
{
	set_free(set), set = 0x0;
	free(reply), reply = 0;
}

void
test_alloc(void)
{
	set = set_alloc();
	ok(set != 0x0);
}

void
test_add(void)
{
	size_t i = 0;

	set = set_alloc();

	for (i = 0; i < array_len(strings); ++i) {
		expect(0, set_add_string(set, strings[i]));
		expect(true, set_contains_string(set, strings[i]));
	}
}

void
test_query(void)
{
	expect(4, set_query_string(0, 0, set, ""));
	expect(1, set_query_string(0x0, 0, set, "foo"));

	set_query_string(&reply, 0, set, "f");

	ok(reply != 0x0);
	ok(reply[0] != 0x0);
	ok(!strcmp(reply[0], "foo"));
	ok(!reply[1]);

	free(reply), reply = 0;

	expect(2, set_query_string(&reply, 0, set, "ba"));

	ok(reply[0] != 0x0);
	ok(reply[1] != 0x0);
	ok(reply[2] == 0x0);
	ok(!strcmp(reply[0], "bar"));
	ok(!strcmp(reply[1], "baz"));
}

void
test_remove(void)
{
	size_t i = 0;

	for (i = 0; i < array_len(strings); ++i) {
		expect(0, set_remove_string(set, strings[i]));
		expect(false, set_contains_string(set, strings[i]));
		expect(0, set_query_string(0x0, 0, set, strings[i]));
	}
}

void
test_prefix(void)
{
	ok(set_prefix_string(set, "f"));
	ok(set_prefix_string(set, "q"));
	ok(set_prefix_string(set, "b"));
}

void
test_fixed(void)
{
	char **arr = calloc(3, sizeof *arr);
	expect(4, set_query_string(&arr, 3, set, ""));
	ok(arr[0] != 0x0);
	ok(arr[1] != 0x0);
	ok(arr[2] == 0x0);
	ok(!strcmp(arr[0], "bar"));
	ok(!strcmp(arr[1], "baz"));
	free(arr);
}

void
test_large_add(void)
{
	char word[256];
	FILE *wordlist;

	wordlist = fopen("opt/words", "r");
	ok(!!wordlist);

	while (fgets(word, 256, wordlist)) {
		expect(0, set_add_string(set, word));
	}

	expect(0, fseek(wordlist, 0, SEEK_SET));

	while (fgets(word, 256, wordlist)) {
		ok(set_contains_string(set, word));
	}
}

void
test_dup(void)
{
	expect(EEXIST, set_add_string(set, "foo"));
}
