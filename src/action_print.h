/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ACTION_PRINT_H
#define _ACTION_PRINT_H

#include "expressions.h"

/**
 * @brief help string for -print
 */
#define expr_action_print_help \
    "    -print\n" \
    "            Print the full file name on the standard output, followed by a\n" \
    "            newline. This is the default action when no action is specified.\n"

/**
 * @brief expr_action_clb implementation for -print action.
 */
enum expr_result expr_action_print_clb(const char *filepath, const char *arg);

/**
 * @brief help string for -print0
 */
#define expr_action_print0_help \
    "    -print0\n" \
    "            Print the full file name on the standard output followed by a null\n" \
    "            character.\n"

/**
 * @brief expr_action_clb implementation for -print0 action.
 */
enum expr_result expr_action_print0_clb(const char *filepath, const char *arg);

#endif /* _ACTION_PRINT_H */
