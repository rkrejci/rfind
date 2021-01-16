/**
 * Copyright (C) 2021 Radek Krejci <radek.krejci@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CMDLINE_H
#define _CMDLINE_H

#include "expressions.h"

/**
 * @brief Parse and store find's symbolic links option -L, -H and -P
 *
 * @param[in] argc Number of command line arguments
 * @param[in] argv Command line arguments
 * @param[in,out] argpos Current index in the @p argv
 * @param[out] options_p Pointer to options storage to be filled.
 * @return EXIT_SUCCESS
 * @return EXIT_FAILURE
 */
int parse_options(int argc, char *argv[], int *argpos, int *options_p);

/**
 * @brief Parse and store find's paths list provided via command line
 *
 * @param[in] argc Number of command line arguments
 * @param[in] argv Command line arguments
 * @param[in,out] argpos Current index in the @p argv
 * @param[out] paths_p Pointer to storage for the created paths array.
 * Caller is supposed to fre it with free().
 * @return EXIT_SUCCESS
 * @return EXIT_FAILURE
 */
int parse_paths(int argc, char *argv[], int *argpos, const char ***paths_p);

/**
 * @brief Parse and store find's expressions provided via command line to filter and do actions on files
 *
 * @param[in] argc Number of command line arguments
 * @param[in] argv Command line arguments
 * @param[in,out] argpos Current index in the @p argv
 * @param[out] expressions_p Pointer to storage for the created paths array.
 * Caller is supposed to free it with expr_free()
 * @return EXIT_SUCCESS
 * @return EXIT_FAILURE
 */
int parse_expressions(int argc, char *argv[], int *argpos, struct expr **expressions_p);

#endif /* _CMDLINE_H */
