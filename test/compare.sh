#!/bin/sh
#
# Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Usage: compare.sh PATH/TO/EXECUTE/rfind

FIND=find
RFIND=$1

TESTDIR1=`dirname $0`/testdir1
TESTDIR2=`dirname $0`/testdir2

# final return value - run all tests, but if any of them fails,
# return non-zero at the end
RESULT=0

compare_finds() {
	touch test_find.out
	touch test_rfind.out

	$FIND $* > test_find.out
	$RFIND $* > test_rfind.out

	if [ `diff test_find.out test_rfind.out | wc -l` -eq 0 ]; then
		echo "TEST OK ($*)"
		return 0
	else
		echo "TEST FAILED ($*)"
		diff test_find.out test_rfind.out
		RESULT=1
		return 1
	fi
}

# create symbolic link in test directory
if [ ! -L ${TESTDIR1}/link ]; then
	ln -s ${TESTDIR2} ${TESTDIR1}/link
fi
# create cyclic symbolic link
if [ ! -L ${TESTDIR2}/selflink ]; then
	ln -s ${TESTDIR2} ${TESTDIR2}/selflink
fi
# create empty directory
if [ ! -d ${TESTDIR1}/emptydir ]; then
	mkdir ${TESTDIR1}/emptydir
fi

#
# list of tests comparing the result of using the same command line options
# by find and rfind
#
# ADD NEW TESTS HERE

# no parameters - default local dir and -print
compare_finds

# testing directory with the default -print
compare_finds ${TESTDIR1}

# testing directory with different symbolic link settings
compare_finds -L ${TESTDIR1}
compare_finds -P ${TESTDIR1}
compare_finds -H ${TESTDIR1}/link
compare_finds -L ${TESTDIR1}/link
compare_finds -P ${TESTDIR1}/link

# simple printing without tests
compare_finds ${TESTDIR1} -print0
compare_finds ${TESTDIR1} -print

# complex expressions
compare_finds ${TESTDIR1} -empty -a -print
compare_finds ${TESTDIR1} -empty -o -print
compare_finds ${TESTDIR1} ! -empty
compare_finds ${TESTDIR1} ! \( -empty -and -print \)
compare_finds ${TESTDIR1} ! \( -empty -or -print \)
compare_finds ${TESTDIR1} \( -empty -o -name "*.txt" \) -a -print0

exit ${RESULT}
