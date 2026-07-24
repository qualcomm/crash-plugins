/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef STRUCT_DEFS_H_
#define STRUCT_DEFS_H_

extern "C" {
    #include "defs.h"
}
#include <iostream>
#include <string>
#include <linux/types.h>
#include <unistd.h>
#include <getopt.h>

#define TO_STRING(x) #x
#define TO_STD_STRING(x) std::string(TO_STRING(x))
#define TO_CONST_STRING(x) const_cast<char*>(x)

class Typeinfo {
public:
    std::string m_name;
    int m_size;
    int m_offset;

    Typeinfo(const std::string& type, const std::string& field, bool is_anon = false)
        : m_name(type + "_" + field),
          m_size(datatype_info(TO_CONST_STRING(type.c_str()), TO_CONST_STRING(field.c_str()), is_anon ? ANON_MEMBER_SIZE_REQUEST : MEMBER_SIZE_REQUEST)),
          m_offset(datatype_info(TO_CONST_STRING(type.c_str()), TO_CONST_STRING(field.c_str()), is_anon ? ANON_MEMBER_OFFSET_REQUEST : NULL)) {}

    Typeinfo(const std::string& type, bool is_anon = false)
        : m_name(type),
          m_size(datatype_info(TO_CONST_STRING(type.c_str()), NULL, STRUCT_SIZE_REQUEST)),
          m_offset(0) {}

    int offset() const {
        return m_offset;
    }

    int size() const {
        return m_size;
    }
};

#endif // STRUCT_DEFS_H_
