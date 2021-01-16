/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TEST_NAME_H
#define _TEST_NAME_H

#include "expressions.h"

/**
 * @brief help string for -name
 */
#define expr_test_name_help \
    "    -name PATTERN\n" \
    "           Filter files by their name matching the shell PATTERN. Only the name\n" \
    "           is matched, not the directory. The metacharacters include `*', `?',\n" \
    "           and `[]'.  Don't forget to enclose the pattern in quotes in order to\n" \
    "           protect it from expansion by the shell.\n"

/**
 * @brief help string for -iname
 */
#define expr_test_iname_help \
    "    -iname PATTERN\n" \
    "            Same as -name, but the match is case insensitive.\n"

/**
 * @brief expr_test_clb implementation for -name test.
 */
enum expr_result expr_test_name_clb(const char *path, const char *name, const struct stat *st, const char *arg);

/**
 * @brief expr_test_clb implementation for -iname test.
 */
enum expr_result expr_test_iname_clb(const char *path, const char *name, const struct stat *st, const char *arg);

#endif /* _TEST_NAME_H */
