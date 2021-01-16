/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TEST_EMPTY_H
#define _TEST_EMPTY_H

#include "expressions.h"

/**
 * @brief help string for -empty
 */
#define expr_test_empty_help \
    "    -empty\n" \
    "            The file is empty.\n"

/**
 * @brief expr_test_clb implementation for -empty test.
 */
enum expr_result expr_test_empty_clb(const char *path, const char *name, const struct stat *st, const char *arg);

#endif /* _TEST_EMPTY_H */
