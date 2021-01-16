/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "action_print.h"

#include "common.h"
#include "expressions.h"

/**
 * -print action: print filepath with newline
 */
enum expr_result
expr_action_print_clb(const char *filepath, const char *UNUSED(arg))
{
    fprintf(stdout, "%s\n", filepath);

    return EXPR_TRUE;
}

/**
 * -print0 action: print filepath without newline
 */
enum expr_result
expr_action_print0_clb(const char *filepath, const char *UNUSED(arg))
{
    fprintf(stdout, "%s%c", filepath, 0);

    return EXPR_TRUE;
}
