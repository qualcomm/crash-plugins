:<<!
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 and
only version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

SPDX-License-Identifier: GPL-2.0-only
!

#!/bin/bash

###############################################################################################################################################
#         1. Run the command 'ulimit -c unlimited' before generating your core dump. This setting is only valid for the current bash session. #
#         2. Retrieve your coredump output path by executing 'cat /proc/sys/kernel/core_pattern'.                                             #
###############################################################################################################################################
CORE_PATH=<coredump path>
LATEST_CORE_FILE=$(ls -t ${CORE_PATH} | head -n 1)
CORE_DUMP=${CORE_PATH}/${LATEST_CORE_FILE}
CRASH_UTILITY_SYM_PATH=<crash_utility symbol path>
GDB_PATH=<x86-64-gdb path>
GDB_CMD_PATH=<cmd.gdb path>
ELF_HEADER=$(od -An -t x1 -N 16 "$CORE_DUMP")
IS_ARM=$(echo $ELF_HEADER | awk '{print $5}')
CRASH_PLUGIN_ARM_SYM=<crash-plugin symbols path>
CRASH_PLUGIN_ARM64_SYM=<crash-plugin symbols path>
function debug_crash_plugin(){
    sed -i '/^\/\//d' ${GDB_CMD_PATH}
    if [ "$IS_ARM" == "02" ]; then
        sed -i '/^set solib-search-path/ s|set solib-search-path.*|set solib-search-path '"${CRASH_PLUGIN_ARM64_SYM}"'|' ${GDB_CMD_PATH}
        ${GDB_PATH} -x ${GDB_CMD_PATH} ${CRASH_UTILITY_SYM_PATH} ${CORE_DUMP}
    elif [ "$IS_ARM" == "01" ]; then
        sed -i '/^set solib-search-path/ s|set solib-search-path.*|set solib-search-path '"${CRASH_PLUGIN_ARM_SYM}"'|' ${GDB_CMD_PATH}
        ${GDB_PATH} -x ${GDB_CMD_PATH} ${CRASH_UTILITY_SYM_PATH} ${CORE_DUMP}
    else
        echo "Unknown architecture"
    fi
}

debug_crash_plugin
