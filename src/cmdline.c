/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "expressions.h"

/**
 * @brief handle global find's options --help and --version.
 *
 * While other command line arguments are divided into groups which cannot mix,
 * these options can appear anywhere.
 *
 * @param[in] arg Command line argument to process, without leading '--'.
 * @return EXIT_SUCCESS
 * @return EXIT_FAILURE for unknown argument
 */
static int
global_options(const char *arg)
{
    if (!strcmp(arg, "help")) {
        fprintf(stdout, "Usage: " FIND_ID " [-H] [-L] [-P] [path...] [expression]\n");
        fprintf(stdout, "\nOPTIONS (the last wins):\n");
        fprintf(stdout, "  -P    Never follow symbolic links. This is the default behavior.\n");
        fprintf(stdout, "  -L    Follow symbolic links.\n");
        fprintf(stdout, "  -H    Follow symbolic link only of the provided paths.\n\n");

        fprintf(stdout, "Default path is the current directory.\n");
        fprintf(stdout, "Default expression is -print, expression may consist of:\n    operators, tests, and actions.\n");

        fprintf(stdout, "\nOPERATORS (decreasing precedence):\n");
        fprintf(stdout, "    ( EXPR )\n"
            "    ! EXPR  -not EXPR\n"
            "    EXPR1 -a EXPR2  EXPR1 -and EXPR2\n"
            "    EXPR1 -o EXPR2  EXPR1 -or EXPR2\n");

        fprintf(stdout, "\nTESTS:\n");
        for (unsigned int i = 0; i < EXPR_TEST_COUNT; i++) {
            fprintf(stdout, expr_tests[i].help);
        }

        fprintf(stdout, "\nACTIONS:\n");
        for (unsigned int i = 0; i < EXPR_ACT_COUNT; i++) {
            fprintf(stdout, expr_actions[i].help);
        }
    } else if (!strcmp(arg, "version")) {
        fprintf(stdout, "Radek's find re-implementation 1.0.0\nCopyright (C) 2021 Radek Krejci\n");
    } else {
        LOG("unknown option --%s", arg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int
parse_options(int argc, char *argv[], int *argpos, int *options_p)
{
    assert(options_p);

    *options_p = EXPR_FOLLOW_NO_SYMLINKS;

    for (; *argpos < argc && argv[*argpos][0] == '-'; (*argpos)++) {
        if (argv[*argpos][1] == '-') {
            global_options(&argv[*argpos][2]);
            return EXIT_FAILURE;
        }

        if (!strcmp(&argv[*argpos][1], "L")) {
            *options_p = EXPR_FOLLOW_SYMLINKS;
        } else if (!strcmp(&argv[*argpos][1], "H")) {
            *options_p = EXPR_FOLLOW_EXPLICIT_SYMLINKS;
        } else if (!strcmp(&argv[*argpos][1], "P")) {
            *options_p = EXPR_FOLLOW_NO_SYMLINKS;
        } else {
            break;
        }
    }

    return EXIT_SUCCESS;
}

int
parse_paths(int argc, char *argv[], int *argpos, const char ***paths_p)
{
    const char **paths = NULL;
    unsigned int paths_count;

    assert(argpos);
    assert(paths_p);

    for (paths_count = 0; *argpos < argc && argv[*argpos][0] != '-'; (*argpos)++, paths_count++) {
        /* possible start characters in expressions */
        if (argv[*argpos][0] == '-' ||
                argv[*argpos][0] == '!' ||
                argv[*argpos][0] == '(') {
            break;
        }
    }
    if (paths_count) {
        paths = malloc((paths_count + 1) * sizeof *paths);
        if (!paths) {
            LOG("%s", strerror(errno));
            return EXIT_FAILURE;
        }
        for (unsigned int i = 0; i < paths_count; i++) {
            paths[i] = argv[*argpos - paths_count + i];
        }
    } else {
        /* the default path is . */
        paths_count = 1;
        paths = malloc((paths_count + 1) * sizeof *paths);
        if (!paths) {
            LOG("%s", strerror(errno));
            return EXIT_FAILURE;
        }
        paths[0] = ".";
    }
    paths[paths_count] = NULL; /* terminating item */
    *paths_p = paths;

    return EXIT_SUCCESS;
}

/**
 * @brief Insert created expression record into postfix list,
 * which will be later converted into the evaluation tree
 *
 * @param[in] expr_new New expression record to be inserted
 * @param[in,out] expressions Head of the expressions list where the new expression is inserted.
 */
static void
expr_list_insert(struct expr *expr_new, struct expr **expressions)
{
    static struct expr *expr_last = NULL;

    if (!(*expressions)) {
        *expressions = expr_last = expr_new;
    } else {
        assert(expr_last);
        expr_last->next = expr_new;
        expr_last = expr_new;
    }
}

/**
 * @brief POP operation on operators stack for converting infix expressions format on command line
 * into internal postfix list for further processing.
 *
 * @param[in] op_stack Operators stack.
 * @param[in, out] count Stack's counter of items inside.
 * return The popped operator.
 */
static enum expr_operator
op_stack_pop(enum expr_operator *op_stack, unsigned int *count)
{
    assert(op_stack);
    assert(*count);

    return op_stack[--(*count)];
}

/**
 * @brief PUSH operation on operators stack for converting infix expressions format on command line
 * into internal postfix list for further processing.
 */
static int
op_stack_push(enum expr_operator op, enum expr_operator **op_stack, unsigned int *size, unsigned int *count,
        struct expr **expressions)
{
#define STACK_STEP 8
    /* first, we have to pop the operators with higher or the same priority,
     * in case of ), all the operators till the opening ( must be processed.
     * All the operators except ( and ) are converted into the expression record
     * and inserted into the expression's postfix representation. */
    if (op != EXPR_OP_LBR) {
        while (*count) {
            enum expr_operator op_prev = op_stack_pop(*op_stack, count);
            if (op == EXPR_OP_RBR) {
                if (op_prev == EXPR_OP_LBR) {
                    /* do not put it back */
                    break;
                }
                /* make the expression record from the popped operator */
                struct expr *expr_new = expr_new_group(op_prev, NULL, NULL);
                expr_list_insert(expr_new, expressions);
            } else if (op_prev >= op) {
                /* make the expression record from the popped operator */
                struct expr *expr_new = expr_new_group(op_prev, NULL, NULL);
                expr_list_insert(expr_new, expressions);
            } else {
                /* put the operator back */
                (*count)++;
                break;
            }
        }
    }
    if (op == EXPR_OP_RBR) {
        /* do nothing, the ) is not being inserted into the stack */
        return EXIT_SUCCESS;
    }

    /* resize the stack, if needed */
    if (*count == *size) {
        void *x = realloc(*op_stack, (*size + STACK_STEP) * sizeof **op_stack);
        if (!x) {
            LOG("%s", strerror(errno));
            return EXIT_FAILURE;
        }
        *op_stack = x;
        *size += STACK_STEP;
    }

    /* insert operator into the stack */
    (*op_stack)[(*count)++] = op;

    return EXIT_SUCCESS;
}

static int
op_stack_clean(enum expr_operator **op_stack, unsigned int *count, struct expr **expressions)
{
    while (*count) {
        enum expr_operator op;
        struct expr *expr_new;

        (*count)--;
        op = (*op_stack)[*count];
        expr_new = expr_new_group(op, NULL, NULL);
        if (!expr_new) {
            free(*op_stack);
            return EXIT_FAILURE;
        }
        expr_list_insert(expr_new, expressions);
    }
    free(*op_stack);
    *op_stack = NULL;

    return EXIT_SUCCESS;
}

int
parse_expressions(int argc, char *argv[], int *argpos, struct expr **expressions_p)
{
    struct expr *expressions = NULL;
    enum expr_operator *op_stack = NULL;
    unsigned int size = 0, count = 0;
    int has_action = 0;

    assert(expressions_p);

    /* The infix representation of the expression on the command line is converted into
     * the postfix list representation, which is then converted into the evaluation tree
     */
    for ( ;*argpos < argc; (*argpos)++) {
        struct expr *expr_new = NULL;

        switch(argv[*argpos][0]) {
        case '-':
            if (argv[*argpos][1] == '-') {
                global_options(&argv[*argpos][2]);
                goto parsing_error;
            }

            /* operators are inserted into the stack and popped and inserted into the postfix
             * list later after all the operands are processed */
            if (!strcmp(&argv[*argpos][1], "not")) {
                op_stack_push(EXPR_OP_NOT, &op_stack, &size, &count, &expressions);
                continue;
            } else if (!strcmp(&argv[*argpos][1], "a") || !strcmp(&argv[*argpos][1], "and")) {
                op_stack_push(EXPR_OP_AND, &op_stack, &size, &count, &expressions);
                continue;
            } else if (!strcmp(&argv[*argpos][1], "o") || !strcmp(&argv[*argpos][1], "or")) {
                op_stack_push(EXPR_OP_OR, &op_stack, &size, &count, &expressions);
                continue;
            }

            /* terminals are tests and actions */
            for (unsigned int i = 0; i < EXPR_TEST_COUNT; i++) {
                if (!strcmp(&argv[*argpos][1], expr_tests[i].id)) {
                    /* match */
                    expr_new = expr_new_test(&expr_tests[i], argc > (*argpos) + 1 ? argv[(*argpos) + 1] : NULL);
                    if (!expr_new) {
                        goto parsing_error;
                    }
                    if (expr_new->test_arg) {
                        (*argpos)++;
                    }
                    goto insert_expr;
                }
            }

            for (unsigned int i = 0; i < EXPR_ACT_COUNT; i++) {
                if (!strcmp(&argv[*argpos][1], expr_actions[i].id)) {
                    /* match */
                    expr_new = expr_new_action(&expr_actions[i], argc > (*argpos) + 1 ? argv[(*argpos) + 1] : NULL);
                    if (!expr_new) {
                        goto parsing_error;
                    }
                    if (expr_new->action_arg) {
                        (*argpos)++;
                    }
                    /* remember we have an action to avoid adding the default one */
                    has_action = 1;
                    goto insert_expr;
                }
            }

            break;
        case '!':
            if (argv[*argpos][1]) {
                LOG("invalid expression %s", argv[*argpos]);
                goto parsing_error;
            }
            op_stack_push(EXPR_OP_NOT, &op_stack, &size, &count, &expressions);
            continue;
        case '(':
            if (argv[*argpos][1]) {
                LOG("invalid expression %s", argv[*argpos]);
                goto parsing_error;
            }
            op_stack_push(EXPR_OP_LBR, &op_stack, &size, &count, &expressions);
            continue;
        case ')':
            if (argv[*argpos][1]) {
                LOG("invalid expression %s", argv[*argpos]);
                goto parsing_error;
            }
            op_stack_push(EXPR_OP_RBR, &op_stack, &size, &count, &expressions);
            continue;

        /* TODO: support , (comma) operator in expression */
        }

insert_expr:
        /* insert the newly created expression into the postfix list and
         * continue with the next item in infix representation */
        if (expr_new) {
            expr_list_insert(expr_new, &expressions);
        } else {
            LOG("invalid expression %s", argv[*argpos]);
            goto parsing_error;
        }
    }

    /* cleanup the rest of the operators stack */
    if (op_stack_clean(&op_stack, &count, &expressions)) {
        goto parsing_error;
    }

    /* now we have postfix list, but we want tree for evaluation -
     * structures are the same, they just need to be interconnected together
     */

    if (!expressions) {
        /* Add a default action, which is -print */
        expressions = expr_new_action(&expr_actions[EXPR_ACT_PRINT], NULL);
        if (!expressions) {
            return EXIT_FAILURE;
        }
    } else {
        /* convert the existing postfix list into the evaluation tree */
        struct expr *e_list = expressions;
        expressions = NULL;
        while (e_list) {
            struct expr *e_grp = NULL;
            for (e_grp = e_list->next; e_grp && (e_grp->type != EXPR_GROUP || e_grp->expr1); e_grp = e_grp->next) ;
            expressions = e_grp ? e_grp : e_list;
            if (e_grp) {
                e_grp->expr1 = e_list;
                if (e_grp->op != EXPR_OP_NOT) {
                    e_grp->expr2 = e_list->next;
                }
            }
            e_list = e_grp;
        }
        if (!has_action) {
            /* default action is -print */
            expressions = expr_new_group(EXPR_OP_AND, expressions, expr_new_action(&expr_actions[EXPR_ACT_PRINT], NULL));
        }
    }

    *expressions_p = expressions;
    return EXIT_SUCCESS;

parsing_error:
    /* cleanup */
    free(op_stack);
    for (struct expr *e = expressions; e; e = expressions) {
        expressions = e->next;
        free(e);
    }
    return EXIT_FAILURE;
}
