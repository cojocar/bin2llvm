/* vim: set expandtab ts=4 sw=4: */

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

#ifndef __JUMP_TABLE_INFO_H__
#define __JUMP_TABLE_INFO_H__ 1

#include <list>
#include <string>
#include <stdint.h>

#include "cajun/json/reader.h"
#include "cajun/json/writer.h"

class JumpTableInfo {
public:
    uint64_t bb_start, bb_end;
    uint64_t indirect_jmp_pc;
    uint64_t default_case_pc;
    uint64_t base_table;
    uint64_t idx_start, idx_stop;
public:
    JumpTableInfo(json::Object o) {
        bb_start = JumpTableInfo::get_uint64_t_from_field(o["bb_start"]);
        bb_end = JumpTableInfo::get_uint64_t_from_field(o["bb_end"]);
        indirect_jmp_pc = JumpTableInfo::get_uint64_t_from_field(o["indirect_jmp_pc"]);
        default_case_pc = JumpTableInfo::get_uint64_t_from_field(o["default_case_pc"]);
        idx_start = JumpTableInfo::get_uint64_t_from_field(o["idx_start"]);
        idx_stop = JumpTableInfo::get_uint64_t_from_field(o["idx_stop"]);
        base_table = JumpTableInfo::get_uint64_t_from_field(o["base_table"]);
    }

    static uint64_t get_uint64_t_from_field(json::UnknownElement e)
    {
        json::Number n = static_cast<json::Number>(e);
        double s = static_cast<double>(n);
        return (uint64_t)s;
    }

};

class JumpTableInfoFactory {
public:
    static std::list<JumpTableInfo *> loadFromFile(std::string);
};

#endif
