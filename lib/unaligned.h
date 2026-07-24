/*
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

#ifndef LIB_UNALIGNED_H_
#define LIB_UNALIGNED_H_

#include "le_byteshift.h"
#include "be_byteshift.h"

#if defined(__LITTLE_ENDIAN)
#define get_unaligned_8 get_unaligned_le8
#define put_unaligned_8 put_unaligned_le8
#define get_unaligned_16 get_unaligned_le16
#define put_unaligned_16 put_unaligned_le16
#define get_unaligned_32 get_unaligned_le32
#define put_unaligned_32 put_unaligned_le32
#define get_unaligned_64 get_unaligned_le64
#define put_unaligned_64 put_unaligned_le64
#else
#define get_unaligned_8 get_unaligned_be8
#define put_unaligned_8 put_unaligned_be8
#define get_unaligned_16 get_unaligned_be16
#define put_unaligned_16 put_unaligned_be16
#define get_unaligned_32 get_unaligned_be32
#define put_unaligned_32 put_unaligned_be32
#define get_unaligned_64 get_unaligned_be64
#define put_unaligned_64 put_unaligned_be64
#endif

#endif // LIB_UNALIGNED_H_
