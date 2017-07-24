

/*
 * Copyright 2017 The bin2llvm Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#include "mem-ops-testing.h"

void
print_memory(const char *e)
{
	int i;
	printf("%s\n", e);
	//for (i = 0; i < sizeof (M); ++i) {
#define L 8
	for (i = 0; i < 128; ++i) {
		if (i % L == 0)
			printf("\n%04x:",  i);
		printf("%02hhx ", (uint8_t)M[i]);
	}
	printf("\n==========================\n");
}

#define P(a) print_memory((a))

int
main()
{
	uint32_t o32 = 0xdeadbeef;
	uint32_t i32 = 0xffffffff;
	uint16_t o16 = 0x1234;
	uint16_t i16 = 0xffff;
	uint8_t o8 = 0xAA;
	uint8_t i8 = 0xff;

	P("before STL");
	STL(0x0, o32);
	P("after STL");
	i32 = LDL(0x0);
	assert((i32 == o32) && "simple stl/ldl failed");

	P("before STL unaligned");
	STL(0x9, o32);
	P("after STL unaligned");
	i32 = LDL(0x9);
	assert((i32 == o32) && "simple stl/ldl failed (unaligned)");

	P("before STW");
	STW(0x20, o16);
	P("after STW");
	i16 = LDW(0x20);
	assert((i16 == o16) && "simple stw/ldw failed");

	P("before STW");
	STW(0x29, o16);
	P("after STW");
	i16 = LDW(0x29);
	assert((i16 == o16) && "simple stw/ldw failed (unaligned)");

	P("before STB");
	STB(0x40, o8);
	P("after STB");
	i8 = LDW(0x40);
	assert((i8 == o8) && "simple stb/ldb failed");

	P("before STB");
	STB(0x49, o8);
	P("after STB");
	i8 = LDW(0x49);
	assert((i8 == o8) && "simple stb/ldb failed (unaligned)");

	((uint32_t *)M)[0x10] = 0xfafbfcfd;
	assert(LDL(0x10 * sizeof (uint32_t)) == 0xfafbfcfd);

	P("final");
	return 0;
}

