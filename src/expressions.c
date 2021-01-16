/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expressions.h"

#include "common.h"

#include "test_name.h"
#include "test_empty.h"

#include "action_print.h"

/**
 * @brief Filled list of information about test modules.
 *
 * ADD NEW MODULES HERE
 */
struct expr_test expr_tests[EXPR_TEST_COUNT] = {
    {.id = "empty", .help = expr_test_empty_help, .test = expr_test_empty_clb, .arg = EXPR_ARG_NO},
    {.id = "iname", .help = expr_test_iname_help, .test = expr_test_iname_clb, .arg = EXPR_ARG_MAND},
    {.id = "name", .help = expr_test_name_help, .test = expr_test_name_clb, .arg = EXPR_ARG_MAND},
};

/**
 * @brief Filled list of information about action modules.
 *
 * ADD NEW MODULES HERE
 */
struct expr_action expr_actions[EXPR_ACT_COUNT] = {
    {.id = "print0", .help = expr_action_print0_help, .action = expr_action_print0_clb, .arg = EXPR_ARG_NO},
    {.id = "print", .help = expr_action_print_help, .action = expr_action_print_clb, .arg = EXPR_ARG_NO},
};

/**
 * @brief Common code for other expr_new_* functions
 * @param[in] type Type of the expression record.
 */
static struct expr *
expr_new(enum expr_type type)
{
    struct expr *e;

    e = calloc(1, sizeof *e);
    if (!e) {
        LOG("%s", strerror(errno));
        return NULL;
    }
    e->type = type;
    return e;
}

struct expr *
expr_new_group(enum expr_operator op, struct expr *e1, struct expr *e2)
{
    struct expr *e;

    if (!(e = expr_new(EXPR_GROUP))) {
        return NULL;
    }
    e->op = op;
    e->expr1 = e1;
    e->expr2 = e2;

    return e;
}

struct expr *
expr_new_test(const struct expr_test *info, const char *arg)
{
    struct expr *e;

    if (!(e = expr_new(EXPR_TEST))) {
        return NULL;
    }
    e->test = info->test;
    if (info->arg == EXPR_ARG_MAND) {
        if (!arg || arg[0] == '-' || arg[0] == '!' || arg[0] == '(' || arg[0] == ')') {
            LOG("missing argument for -%s test.", info->id);
            free(e);
            return NULL;
        }
        e->test_arg = arg;
    } else if (info->arg == EXPR_ARG_OPT && arg && arg[0] != '-' && arg[0] != '!' && arg[0] != '(' && arg[0] != ')') {
        e->test_arg = arg;
    } else if (info->arg == EXPR_ARG_NO && arg && arg[0] != '-' && arg[0] != '!' && arg[0] != '(' && arg[0] != ')') {
        LOG("invalid argument for -%s test.", info->id);
        free(e);
        return NULL;
    }

    return e;
}

struct expr *
expr_new_action(const struct expr_action *info, const char *arg)
{
    struct expr *e;

    if (!(e = expr_new(EXPR_ACT))) {
        return NULL;
    }
    e->action = info->action;
    if (info->arg == EXPR_ARG_MAND) {
        if (!arg || arg[0] == '-' || arg[0] == '!' || arg[0] == '(' || arg[0] == ')') {
            LOG("missing argument for -%s action.", info->id);
            free(e);
            return NULL;
        }
        e->action_arg = arg;
    } else if (info->arg == EXPR_ARG_OPT && arg && arg[0] != '-' && arg[0] != '!' && arg[0] != '(' && arg[0] != ')') {
        e->action_arg = arg;
    } else if (info->arg == EXPR_ARG_NO && arg && arg[0] != '-' && arg[0] != '!' && arg[0] != '(' && arg[0] != ')') {
        LOG("invalid argument for -%s action.", info->id);
        free(e);
        return NULL;
    }

    return e;
}

enum expr_result
expr_eval(const char *filepath, const char *name, struct stat *st, struct expr *expr)
{
    enum expr_result r1, r2;

    switch(expr->type) {
    case EXPR_GROUP:
        r1 = expr_eval(filepath, name, st, expr->expr1);
        if (expr->op == EXPR_OP_NOT) {
            return r1 ? EXPR_FALSE : EXPR_TRUE;
        } else {
            if (expr->op == EXPR_OP_AND && !r1) {
                return r1;
            } else if (expr->op == EXPR_OP_OR && r1) {
                return r1;
            }
            r2 = expr_eval(filepath, name, st, expr->expr2);
            if (expr->op == EXPR_OP_AND) {
                return r1 && r2;
            } else if (expr->op == EXPR_OP_OR) {
                return r1 || r2;
            }
        }
        break;
    case EXPR_TEST:
        return expr->test(filepath, name, st, expr->action_arg);
    case EXPR_ACT:
        return expr->action(filepath, expr->action_arg);
    }

    return EXPR_FALSE;
}

void
expr_free(struct expr *e)
{
    if (!e) {
        return;
    }

    if (e->type == EXPR_GROUP) {
        expr_free(e->expr1);
        expr_free(e->expr2);
    }
    free(e);
}
