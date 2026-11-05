#pragma once
void test_fail(const char *);
#define assert(x) ((x)?(void)0:test_fail(#x))
