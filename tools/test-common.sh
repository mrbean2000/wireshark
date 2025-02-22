#!/bin/bash
#
# Copyright 2013 Gerald Combs <gerald@wireshark.org>
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Common variables and functions for fuzz and randpkt tests.

# This needs to point to a 'date' that supports %s.
if [ -z "$TEST_TYPE" ] ; then
    echo "TEST_TYPE must be defined by the sourcing script."
    exit 1
fi

DATE=/bin/date
BASE_NAME=$TEST_TYPE-$($DATE +%Y-%m-%d)-$$

# Directory containing binaries.  Default: cmake run directory.
if [ -z "$WIRESHARK_BIN_DIR" ]; then
    WIRESHARK_BIN_DIR=run
fi

# Temporary file directory and names.
# (had problems with this on cygwin, tried TMP_DIR=./ which worked)
TMP_DIR=/tmp
if [ "$OSTYPE" == "cygwin" ] ; then
        TMP_DIR=$(cygpath --windows "$TMP_DIR")
fi
TMP_FILE=$BASE_NAME.pcap
ERR_FILE=$BASE_NAME.err

# Loop this many times (< 1 loops forever)
MAX_PASSES=0

# These may be set to your liking
# Stop the child process if it's running longer than x seconds
MAX_CPU_TIME=600
# Stop the child process if it's using more than y * 1024 bytes
MAX_VMEM=1000000
# Stop the child process if its stack is larger than than z * 1024 bytes
# Windows XP:    2033
# Windows 7:     2034
# Mac OS X 10.6: 8192
# Linux 2.6.24:  8192
# Solaris 10:    8192
MAX_STACK=2033
# Insert z times an error into the capture file (0.02 seems to be a good value to find errors)
ERR_PROB=0.02

# Call *after* any changes to WIRESHARK_BIN_DIR (e.g., via command-line options)
function ws_bind_exec_paths() {
# Tweak the following to your liking.  Editcap must support "-E".
TSHARK="$WIRESHARK_BIN_DIR/tshark"
EDITCAP="$WIRESHARK_BIN_DIR/editcap"
CAPINFOS="$WIRESHARK_BIN_DIR/capinfos"
RANDPKT="$WIRESHARK_BIN_DIR/randpkt"

if [ "$WIRESHARK_BIN_DIR" = "." ]; then
    export WIRESHARK_RUN_FROM_BUILD_DIRECTORY=1
fi
}

function ws_check_exec() {
NOTFOUND=0
for i in "$@" ; do
    if [ ! -x "$i" ]; then
        echo "Couldn't find \"$i\""
        NOTFOUND=1
    fi
done
if [ $NOTFOUND -eq 1 ]; then
    exit 1
fi
}

##############################################################################
### Set up environment variables for fuzz testing			   ###
##############################################################################
# Use the Wmem strict allocator which does canaries and scrubbing etc.
export WIRESHARK_DEBUG_WMEM_OVERRIDE=strict
# Abort if a dissector adds too many items to the tree
export WIRESHARK_ABORT_ON_TOO_MANY_ITEMS=

# Turn on GLib memory debugging (since 2.13)
export G_SLICE=debug-blocks
# Cause glibc (Linux) to abort() if some memory errors are found
export MALLOC_CHECK_=3
# Cause FreeBSD (and other BSDs) to abort() on allocator warnings and
# initialize allocated memory (to 0xa5) and freed memory (to 0x5a).  see:
# https://www.freebsd.org/cgi/man.cgi?query=malloc&apropos=0&sektion=0&manpath=FreeBSD+8.2-RELEASE&format=html
export MALLOC_OPTIONS=AJ

# macOS options; see https://developer.apple.com/library/archive/documentation/Performance/Conceptual/ManagingMemory/Articles/MallocDebug.html
# Initialize allocated memory to 0xAA and freed memory to 0x55
export MallocPreScribble=1
export MallocScribble=1
# Add guard pages before and after large allocations
export MallocGuardEdges=1
# Call abort() if heap corruption is detected.  Heap is checked every 1000
# allocations (may need to be tuned!)
export MallocCheckHeapStart=1000
export MallocCheckHeapEach=1000
export MallocCheckHeapAbort=1
# Call abort() if an illegal free() call is made
export MallocBadFreeAbort=1

# Address Sanitizer options
export ASAN_OPTIONS=detect_leaks=0

# See if we were configured with gcc or clang's AddressSanitizer.
CONFIGURED_WITH_ASAN=0
# If tshark is built with ASAN this will generate an error. We could
# also pass help=1 and look for help text.
ASAN_OPTIONS=Invalid_Option_Flag $TSHARK -h > /dev/null 2>&1
if [ $? -ne 0 ] ; then
    CONFIGURED_WITH_ASAN=1
fi
export CONFIGURED_WITH_ASAN

# Create an error report
function ws_exit_error() {
    echo -e "\n ERROR"
    echo -e "Processing failed. Capture info follows:\n"
    echo "  Input file: $CF"
    echo "  Output file: $TMP_DIR/$TMP_FILE"
    echo "  Pass: $PASS"
    echo

    # Fill in build information
    {
        echo -e "Branch: $(git rev-parse --abbrev-ref HEAD)\n"
        echo -e "Input file: $CF\n"
        echo -e "Build host information:"
        uname -srvm
        lsb_release -a 2> /dev/null

        if [ -n "$CI_JOB_NAME" ] ; then
            echo -e "\nCI job name: $CI_JOB_NAME, ID: $CI_JOB_ID "
        fi

        echo -e "\nReturn value: " $RETVAL
        echo -e "\nDissector bug: " $DISSECTOR_BUG
        echo -e "\nValgrind error count: " $VG_ERR_CNT

        if [ -d "${GIT_DIR:-.git}" ] ; then
                echo -e "\nLatest (but not necessarily the problem) commit:"
                git log --max-count=1 --oneline
        fi
    } > "$TMP_DIR/${ERR_FILE}.header"

    # Trim the stderr output if needed
    ERR_SIZE=$(du -sk $TMP_DIR/$ERR_FILE | awk '{ print $1 }')
    if [ $ERR_SIZE -ge 5000 ] ; then
        mv $TMP_DIR/$ERR_FILE $TMP_DIR/${ERR_FILE}.full
        head -n 2000 $TMP_DIR/${ERR_FILE}.full > $TMP_DIR/$ERR_FILE
        echo -e "\n\n[ Output removed ]\n\n" >> $TMP_DIR/$ERR_FILE
        tail -n 2000 $TMP_DIR/${ERR_FILE}.full >> $TMP_DIR/$ERR_FILE
        rm -f $TMP_DIR/${ERR_FILE}.full
    fi

    cat $TMP_DIR/${ERR_FILE} >> $TMP_DIR/${ERR_FILE}.header
    mv $TMP_DIR/${ERR_FILE}.header $TMP_DIR/${ERR_FILE}

    echo -e "stderr follows:\n"
    cat $TMP_DIR/$ERR_FILE

    exit 255
}
