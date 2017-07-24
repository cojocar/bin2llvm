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

#include "JumpTableInfo.h"

#include <list>
#include <string>
#include <iostream>
#include <fstream>
#include <assert.h>

std::list<JumpTableInfo *>
JumpTableInfoFactory::loadFromFile(std::string fname)
{
    std::cout << "[JumpTableInfo] loading from file " << fname << std::endl;
    std::list<JumpTableInfo *> ret;
    std::cout << fname << std::endl;

    std::istream *stream = new
        std::ifstream(fname.c_str(), std::ios::in |
                std::ios::binary);
    assert(stream);

    json::Object root;
    json::Reader::Read(root, *stream);
    json::Object::const_iterator funcName(root.Begin()),
        funcNameEnd(root.End());
    for (; funcName != funcNameEnd; ++funcName) {
        std::cout << funcName->name << std::endl;
        json::Array t = static_cast<json::Array>(root[funcName->name]);
        json::Array::const_iterator ti(t.Begin()), te(t.End());
        for ( ; ti != te; ++ti) {
            json::Object oneJump = static_cast<json::Object>(*ti);
            JumpTableInfo *o = new JumpTableInfo(oneJump);
            ret.push_back(o);
        }
    }
    std::cout << "[JumpTableInfo] loaded " << ret.size() << " tables" <<
        std::endl;
    return ret;
}
