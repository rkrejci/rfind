/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _EXPRESSIONS_H
#define _EXPRESSIONS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define EXPR_FOLLOW_NO_SYMLINKS 0x0        /**< do not follow symlinks at all, default behavior */
#define EXPR_FOLLOW_EXPLICIT_SYMLINKS 0x1  /**< follow symlinks only in case of explcitly provided paths */
#define EXPR_FOLLOW_SYMLINKS 0x3           /**< follow all symlinks, option -L */

/**
 * @brief Accepted operators in expressions
 */
enum expr_operator {
    EXPR_OP_LBR,  /* ( */
    EXPR_OP_RBR,  /* ) */
    EXPR_OP_OR,   /* -o, -or */
    EXPR_OP_AND,  /* -a, -and */
    EXPR_OP_NOT,  /* !, -not */
};

/**
 * @brief Types of expression records.
 */
enum expr_type {
    EXPR_GROUP,
    EXPR_TEST,
    EXPR_ACT
};

/**
 * @brief Possible results when evaluating expression (tests and actions)
 */
enum expr_result {
    EXPR_FALSE = 0,  /**< false */
    EXPR_TRUE = 1    /**< true */
};

/**
 * @brief Possible argument's presence expectations for tests and actions
 */
enum expr_arg {
    EXPR_ARG_MAND,  /**< mandatory argument */
    EXPR_ARG_OPT,   /**< optional argument */
    EXPR_ARG_NO     /**< no argument expected */
};

/**
 * @brief Callback for executing find tests
 *
 * @param[in] filepath Path of the file being tested
 * @param[in] name Name of the file being tested
 * @param[in] st File information
 * @param[in] arg Argument of the test, can be NULL in case there is no argument on command line
 *
 * @return EXPR_FALSE for false result
 * @return EXPR_TRUE for true result
 */
typedef enum expr_result (*expr_test_clb)(const char *filepath, const char *name, const struct stat *st, const char *arg);

/**
 * @brief List of available test module indexes in expr_tests.
 *
 * ADD NEW MODULES HERE
 */
enum expr_test_id {
    EXPR_TEST_EMPTY = 0,   /**< -empty */
    EXPR_TEST_INAME,       /**< -iname */
    EXPR_TEST_NAME,        /**< -name */

    EXPR_TEST_COUNT        /**< total number of available tests */
};

/**
 * @brief Information about specific test module to allow its usage.
 */
struct expr_test {
    const char *id;        /**< identifier - name of the command line option */
    const char *help;      /**< help string */
    expr_test_clb test;    /**< test callback */
    enum expr_arg arg;     /**< hint about the test's argument presence */
};

/**
 * @brief List of information about available test modules.
 */
extern struct expr_test expr_tests[EXPR_TEST_COUNT];

/**
 * @brief Callback for executing find tests
 *
 * @param[in] filepath Path of the file being processed
 * @param[in] arg Argument of the test, can be NULL in case there is no argument on command line
 *
 * @return EXPR_FALSE when the action fails
 * @return EXPR_TRUE when the action succeeds
 */
typedef enum expr_result (*expr_action_clb)(const char *filepath, const char *arg);

/**
 * @brief List of available action module indexes in expr_actions.
 *
 * ADD NEW MODULES HERE
 */
enum expr_action_id {
    EXPR_ACT_PRINT0 = 0,  /**< -print0 */
    EXPR_ACT_PRINT,       /**< -print */

    EXPR_ACT_COUNT        /**< total number of available tests */
};

/**
 * @brief Information about specific action module to allow its usage.
 */
struct expr_action {
    const char *id;           /**< identifier - name of the command line option */
    const char *help;         /**< help string */
    expr_action_clb action;   /**< action callback */
    enum expr_arg arg;        /**< hint about the action's argument presence */
};

/**
 * @brief List of information about available action modules.
 */
extern struct expr_action expr_actions[EXPR_ACT_COUNT];

/**
 * @brief Expression record
 */
struct expr {
    enum expr_type type;             /**< Type of the expression record,
                                          The following union is processed according to this value */
    union {
        struct {
            enum expr_operator op;   /**< operand modifying subexpression(s) */
            struct expr *expr1;      /**< first subexpression */
            struct expr *expr2;      /**< second subexpression */
        };                           /**< members for EXPR_GROUP type */
        struct {
            expr_test_clb test;      /**< test callback */
            const char *test_arg;    /**< test's argument */
        };                           /**< members for EXPR_TEST type */
        struct {
            expr_action_clb action;  /**< action callback */
            const char *action_arg;  /**< action's argument */
        };                           /**< members for EXPR_ACT type */
    };

    struct expr *next;               /**< aux pointer to the next expression in the postfix list */
};

/**
 * @brief Create new expression record for the given operator.
 * @param[in] op Operator of the expression record.
 * @param[in] e1 First operand (if available, NULL is accepted)
 * @param[in] e2 Second operand (if applicable and available, NULL is accepted)
 * @return NULL in case of failure.
 * @return pointer to the created expression record.
 */
struct expr *expr_new_group(enum expr_operator op, struct expr *e1, struct expr *e2);

/**
 * @brief Create new expression record for the test terminal.
 * @param[in] info Information about the test module
 * @param[in] arg Argument of the test, is checked according to the information in @p info
 * @return NULL in case of failure.
 * @return pointer to the created expression record.
 */
struct expr *expr_new_test(const struct expr_test *info, const char *arg);

/**
 * @brief Create new expression record for the action terminal.
 * @param[in] info Information about the action module
 * @param[in] arg Argument of the action, is checked according to the information in @p info
 * @return NULL in case of failure.
 * @return pointer to the created expression record.
 */
struct expr *expr_new_action(const struct expr_action *info, const char *arg);

/**
 * @brief Evaluate the expression evaluation tree on the file of given attributes.
 *
 * @param[in] filepath Path of the file being processed.
 * @param[in] name The name (basename) of the file being processed.
 * @param[in] st The stat information.
 * @param[in] expr The evaluation tree of the expression.
 * @return EXPR_FALSE or EXPR_TRUE according to the result of evaluating expression on the file.
 */
enum expr_result expr_eval(const char *filepath, const char *name, struct stat *st, struct expr *expr);

/**
 * @brief Free the expressions evaluation tree.
 * @param[in] e Root of the evaluation tree to be freed.
 */
void expr_free(struct expr *e);

#endif /* _EXPRESSIONS_H  */
