/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fnmatch.h>
#include <stdio.h>

#include "test_name.h"

#include "common.h"

/**
 * @brief Common code for the name test, they differ just by flags for fnmatch()
 *
 * @param[in] action Name of the action for logging.
 * @param[in] name Name of the file to match.
 * @param[in] pattern Pattern string to evaluate.
 * @param[in] flags Flags for fnmatch()
 *
 * @return EXPR_FALSE when the @p name does not match the @p pattern
 * @return EXPR_TRUE when the @p name match the @p pattern
 */
static enum expr_result
expr_test_name_common(const char *action, const char *name, const char *pattern, int flags)
{
    int rc;

    rc = fnmatch(pattern, name, flags);
    if (!rc) {
        return EXPR_TRUE;
    } else if (rc == FNM_NOMATCH) {
        return EXPR_FALSE;
    } else {
        LOG("invalid pattern (%s) for -%s action.", pattern, action);
        return EXPR_FALSE;
    }
}

enum expr_result
expr_test_name_clb(const char *UNUSED(path), const char *name, const struct stat *UNUSED(st), const char *arg)
{
    return expr_test_name_common("name", name, arg, 0);
}

enum expr_result
expr_test_iname_clb(const char *UNUSED(path), const char *name, const struct stat *UNUSED(st), const char *arg)
{
    return expr_test_name_common("iname", name, arg, FNM_CASEFOLD);
}
