#ifndef __MEM_OPS_H__
#define __MEM_OPS_H__ 1
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
#define __MEM_OPS_H__ 1

#include <stdint.h>

extern int32_t __ldl_mmu(int32_t addr, int32_t __unused);
extern int16_t __ldw_mmu(int32_t addr, int32_t __unused);
extern int8_t  __ldb_mmu(int32_t addr, int32_t __unused);
#define LDL(A) __ldl_mmu((A), 0)
#define LDW(A) __ldw_mmu((A), 0)
#define LDB(A) __ldb_mmu((A), 1)

extern void __stl_mmu(int32_t addr, int32_t val, int32_t __unused);
extern void __stw_mmu(int32_t addr, int32_t val, int32_t __unused);
extern void __stb_mmu(int32_t addr, int32_t val, int32_t __unused);

#define STL(A, V) __stl_mmu((A), (V), 0)
#define STW(A, V) __stw_mmu((A), (V), 0)
#define STB(A, V) __stb_mmu((A), (V), 0)

extern uint8_t MEMORY[4096];
#define M MEMORY

#endif
